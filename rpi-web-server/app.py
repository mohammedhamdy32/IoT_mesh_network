import asyncio
import json
import logging
import os
import sqlite3
import uuid
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Union

import paho.mqtt.client as mqtt
from fastapi import Depends, FastAPI, HTTPException, Request, WebSocket, WebSocketDisconnect, status
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse, RedirectResponse
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from fastapi.staticfiles import StaticFiles
from jose import JWTError, jwt
from passlib.context import CryptContext
from pydantic import BaseModel

# Event loop for asyncio
# Ensure the event loop is created for the current thread
loop = asyncio.get_event_loop()
if loop is None:  # If no loop is present in the current thread
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)
logger = logging.getLogger("wot_system")

# MQTT Configuration
MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_SENSOR_TOPIC = "wot/sensors/#"
MQTT_CONTROL_TOPIC = "wot/control/#"

# JWT Configuration
SECRET_KEY = os.getenv("SECRET_KEY", "Badawy_random_secret_key")
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 30
REFRESH_TOKEN_EXPIRE_DAYS = 30

# SQLite Database Configuration
DB_FILE = "wot_data.db"

# Password hashing
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")

# OAuth2 scheme
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="api/auth/token")

# Initialize FastAPI app
app = FastAPI(title="Web of Things API", description="API for WoT system with MQTT integration")

# Configure CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # In production, replace with specific origins
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Data models
class Token(BaseModel):
    access_token: str
    refresh_token: str
    token_type: str


class TokenData(BaseModel):
    username: Optional[str] = None
    permissions: Optional[List[str]] = None


class User(BaseModel):
    username: str
    email: Optional[str] = None
    full_name: Optional[str] = None
    disabled: Optional[bool] = False
    permissions: List[str] = []


class UserInDB(User):
    hashed_password: str


class UserCreate(BaseModel):
    username: str
    email: str
    password: str
    full_name: Optional[str] = None


class RefreshToken(BaseModel):
    refresh_token: str


class SensorData(BaseModel):
    node_id: str
    temperature: Optional[float] = None
    humidity: Optional[float] = None
    light: Optional[float] = None
    pressure: Optional[float] = None
    timestamp: Optional[str] = None


class ControlCommand(BaseModel):
    node_id: str
    action: str
    value: float


# WebSocket connection manager
class ConnectionManager:
    def __init__(self):
        self.active_connections: Dict[str, List[WebSocket]] = {}  # Map username to connections

    async def connect(self, websocket: WebSocket, user_id: str):
        await websocket.accept()
        if user_id not in self.active_connections:
            self.active_connections[user_id] = []
        self.active_connections[user_id].append(websocket)
        logger.info(f"New WebSocket connection for user {user_id}. Total connections: {len(self.active_connections[user_id])}")

    def disconnect(self, websocket: WebSocket, user_id: str):
        if user_id in self.active_connections:
            if websocket in self.active_connections[user_id]:
                self.active_connections[user_id].remove(websocket)
                logger.info(f"WebSocket disconnected for user {user_id}. Remaining connections: {len(self.active_connections[user_id])}")
            if not self.active_connections[user_id]:
                del self.active_connections[user_id]

    async def broadcast(self, message: Dict, user_id: Optional[str] = None):
        if user_id:
            # Broadcast to specific user
            connections = self.active_connections.get(user_id, [])
            for connection in connections:
                try:
                    await connection.send_json(message)
                except Exception as e:
                    logger.error(f"Error broadcasting to WebSocket for user {user_id}: {e}")
        else:
            # Broadcast to all users
            for user_id, connections in self.active_connections.items():
                for connection in connections:
                    try:
                        await connection.send_json(message)
                    except Exception as e:
                        logger.error(f"Error broadcasting to WebSocket for user {user_id}: {e}")


manager = ConnectionManager()

# Authentication functions
def verify_password(plain_password, hashed_password):
    return pwd_context.verify(plain_password, hashed_password)


def get_password_hash(password):
    return pwd_context.hash(password)


def get_user(username: str):
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()
    
    cursor.execute("SELECT * FROM users WHERE username = ?", (username,))
    user_data = cursor.fetchone()
    
    conn.close()
    
    if user_data:
        # Convert permissions from comma-separated string to list
        permissions = user_data["permissions"].split(",") if user_data["permissions"] else []
        return UserInDB(
            username=user_data["username"],
            email=user_data["email"],
            full_name=user_data["full_name"],
            disabled=bool(user_data["disabled"]),
            permissions=permissions,
            hashed_password=user_data["hashed_password"]
        )


def authenticate_user(username: str, password: str):
    user = get_user(username)
    if not user:
        return False
    if not verify_password(password, user.hashed_password):
        return False
    return user


def create_access_token(data: dict, expires_delta: Optional[timedelta] = None):
    to_encode = data.copy()
    if expires_delta:
        expire = datetime.utcnow() + expires_delta
    else:
        expire = datetime.utcnow() + timedelta(minutes=15)
    to_encode.update({"exp": expire})
    encoded_jwt = jwt.encode(to_encode, SECRET_KEY, algorithm=ALGORITHM)
    return encoded_jwt


def create_refresh_token():
    return str(uuid.uuid4())


def store_refresh_token(refresh_token: str, username: str):
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    # Delete existing refresh tokens for this user
    cursor.execute("DELETE FROM refresh_tokens WHERE username = ?", (username,))
    
    # Store new refresh token
    expire_date = datetime.utcnow() + timedelta(days=REFRESH_TOKEN_EXPIRE_DAYS)
    cursor.execute(
        "INSERT INTO refresh_tokens (token, username, expires_at) VALUES (?, ?, ?)",
        (refresh_token, username, expire_date.isoformat())
    )
    
    conn.commit()
    conn.close()


def validate_refresh_token(refresh_token: str):
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()
    
    cursor.execute(
        "SELECT * FROM refresh_tokens WHERE token = ? AND expires_at > ?",
        (refresh_token, datetime.utcnow().isoformat())
    )
    token_data = cursor.fetchone() # Get the token data if it exists and is not expired
    
    conn.close()
    
    if token_data:
        return token_data["username"]
    return None


async def get_current_user(token: str = Depends(oauth2_scheme)):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        username: str = payload.get("sub")
        if username is None:
            raise credentials_exception
        token_data = TokenData(username=username, permissions=payload.get("permissions", []))
    except JWTError:
        raise credentials_exception
    user = get_user(username=token_data.username)
    if user is None:
        raise credentials_exception
    return user


async def get_current_active_user(current_user: User = Depends(get_current_user)):
    if current_user.disabled:
        raise HTTPException(status_code=400, detail="Inactive user")
    return current_user


def check_permission(required_permission: str):
    async def permission_dependency(current_user: User = Depends(get_current_active_user)):
        if "admin" in current_user.permissions or required_permission in current_user.permissions:
            return current_user
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Insufficient permissions",
        )
    return permission_dependency


# Database setup
def init_db():
    """Initialize the SQLite database with required tables"""
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    # Create sensor data table
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS sensor_data (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        node_id TEXT NOT NULL,
        temperature REAL,
        humidity REAL,
        light REAL,
        pressure REAL,
        timestamp TEXT NOT NULL
    )
    ''')
    
    # Create control commands table
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS control_commands (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        node_id TEXT NOT NULL,
        action TEXT NOT NULL,
        value REAL NOT NULL,
        timestamp TEXT NOT NULL
    )
    ''')
    
    # Create users table
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        email TEXT UNIQUE NOT NULL,
        full_name TEXT,
        hashed_password TEXT NOT NULL,
        disabled BOOLEAN DEFAULT 0,
        permissions TEXT
    )
    ''')
    
    # Create refresh tokens table
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS refresh_tokens (
        token TEXT PRIMARY KEY,
        username TEXT NOT NULL,
        expires_at TEXT NOT NULL,
        FOREIGN KEY (username) REFERENCES users (username)
    )
    ''')
    
    # Create default admin user if no users exist
    cursor.execute("SELECT COUNT(*) FROM users")
    if cursor.fetchone()[0] == 0:
        admin_password = os.getenv("ADMIN_PASSWORD", "admin123")  # Should be changed in production
        hashed_password = get_password_hash(admin_password)
        cursor.execute(
            "INSERT INTO users (username, email, full_name, hashed_password, permissions) VALUES (?, ?, ?, ?, ?)",
            ("admin", "admin@example.com", "Administrator", hashed_password, "admin")
        )
        logger.info("Created default admin user. Please change the password immediately!")
    
    conn.commit()
    conn.close()
    logger.info("Database initialized successfully")


# MQTT client setup
def on_connect(client, userdata, flags, rc):
    """Callback for when the client connects to the MQTT broker"""
    if rc == 0:
        logger.info("Connected to MQTT broker")
        client.subscribe(MQTT_SENSOR_TOPIC)
        logger.info(f"Subscribed to {MQTT_SENSOR_TOPIC}")
    else:
        logger.error(f"Failed to connect to MQTT broker with code {rc}")


def on_message(client, userdata, msg):
    """Callback for when a message is received from the MQTT broker"""
    try:
        topic = msg.topic
        payload = json.loads(msg.payload.decode())
        logger.info(f"Received message on topic {topic}: {payload}")
        
        # Parse node_id from topic (format: wot/sensors/node_id)
        node_id = topic.split("/")[-1]
        
        # Add timestamp if not present
        if "timestamp" not in payload:
            payload["timestamp"] = datetime.now().isoformat()
            
        # Add node_id if not present
        if "node_id" not in payload:
            payload["node_id"] = node_id
        
        # Store in SQLite
        store_sensor_data(payload)
        
        # Broadcast to WebSocket clients
        asyncio.run_coroutine_threadsafe(
            manager.broadcast({"type": "sensor_data", "data": payload}),
            loop
        )
    except Exception as e:
        logger.error(f"Error processing MQTT message: {e}")


def store_sensor_data(data):
    """Store sensor data in SQLite database"""
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    cursor.execute(
        """
        INSERT INTO sensor_data (node_id, temperature, humidity, light, pressure, timestamp)
        VALUES (?, ?, ?, ?, ?, ?)
        """,
        (
            data.get("node_id"),
            data.get("temperature"),
            data.get("humidity"),
            data.get("light"),
            data.get("pressure"),
            data.get("timestamp")
        )
    )
    
    conn.commit()
    conn.close()


def store_control_command(data):
    """Store control command in SQLite database"""
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    cursor.execute(
        """
        INSERT INTO control_commands (node_id, action, value, timestamp)
        VALUES (?, ?, ?, ?)
        """,
        (
            data.get("node_id"),
            data.get("action"),
            data.get("value"),
            datetime.now().isoformat()
        )
    )
    
    conn.commit()
    conn.close()


# Initialize MQTT client
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message


# Auth endpoints
@app.post("/api/auth/token", response_model=Token)
async def login_for_access_token(form_data: OAuth2PasswordRequestForm = Depends()):
    user = authenticate_user(form_data.username, form_data.password)
    if not user:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password",
            headers={"WWW-Authenticate": "Bearer"},
        )
    access_token_expires = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(
        data={"sub": user.username, "permissions": user.permissions},
        expires_delta=access_token_expires
    )
    refresh_token = create_refresh_token()
    store_refresh_token(refresh_token, user.username)
    
    return {"access_token": access_token, "refresh_token": refresh_token, "token_type": "bearer"}


@app.post("/api/auth/refresh", response_model=Token)
async def refresh_access_token(token_data: RefreshToken):
    username = validate_refresh_token(token_data.refresh_token)
    if not username:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid or expired refresh token",
            headers={"WWW-Authenticate": "Bearer"},
        )
    
    user = get_user(username)
    if not user:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="User not found",
            headers={"WWW-Authenticate": "Bearer"},
        )
    
    access_token_expires = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(
        data={"sub": user.username, "permissions": user.permissions},
        expires_delta=access_token_expires
    )
    new_refresh_token = create_refresh_token()
    store_refresh_token(new_refresh_token, user.username)
    
    return {"access_token": access_token, "refresh_token": new_refresh_token, "token_type": "bearer"}


@app.post("/api/auth/register", status_code=status.HTTP_201_CREATED)
async def register_user(user_data: UserCreate, current_user: User = Depends(check_permission("admin"))):
    # Only admins can create new users
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    # Check if username already exists
    cursor.execute("SELECT COUNT(*) FROM users WHERE username = ?", (user_data.username,))
    if cursor.fetchone()[0] > 0:
        conn.close()
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Username already registered",
        )
    
    # Check if email already exists
    cursor.execute("SELECT COUNT(*) FROM users WHERE email = ?", (user_data.email,))
    if cursor.fetchone()[0] > 0:
        conn.close()
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Email already registered",
        )
    
    # Create new user
    hashed_password = get_password_hash(user_data.password)
    cursor.execute(
        """
        INSERT INTO users (username, email, full_name, hashed_password, permissions)
        VALUES (?, ?, ?, ?, ?)
        """,
        (
            user_data.username,
            user_data.email,
            user_data.full_name,
            hashed_password,
            "user"  # Default permission
        )
    )
    
    conn.commit()
    conn.close()
    
    return {"message": "User registered successfully"}


@app.get("/api/auth/me", response_model=User)
async def read_users_me(current_user: User = Depends(get_current_active_user)):
    return current_user


@app.put("/api/auth/change-password")
async def change_password(
    old_password: str,
    new_password: str,
    current_user: User = Depends(get_current_active_user)
):
    # Verify old password
    user = authenticate_user(current_user.username, old_password)
    if not user:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect password",
        )
    
    # Update password
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    hashed_password = get_password_hash(new_password)
    cursor.execute(
        "UPDATE users SET hashed_password = ? WHERE username = ?",
        (hashed_password, current_user.username)
    )
    
    # Invalidate all refresh tokens
    cursor.execute("DELETE FROM refresh_tokens WHERE username = ?", (current_user.username,))
    
    conn.commit()
    conn.close()
    
    return {"message": "Password changed successfully"}


# REST API endpoints
@app.get("/api/latest")
async def get_latest_readings(current_user: User = Depends(get_current_active_user)):
    """Get the latest sensor readings for all nodes"""
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()
    
    cursor.execute(
        """
        WITH ranked_data AS (
            SELECT 
                *,
                ROW_NUMBER() OVER(PARTITION BY node_id ORDER BY timestamp DESC) as rn
            FROM sensor_data
        )
        SELECT node_id, temperature, humidity, light, pressure, timestamp
        FROM ranked_data
        WHERE rn = 1
        """
    )
    
    results = cursor.fetchall()
    conn.close()
    
    return [dict(row) for row in results]


@app.get("/api/history/{node_id}")
async def get_node_history(
    node_id: str,
    limit: int = 100,
    current_user: User = Depends(get_current_active_user)
):
    """Get historical sensor data for a specific node"""
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()
    
    cursor.execute(
        """
        SELECT node_id, temperature, humidity, light, pressure, timestamp
        FROM sensor_data
        WHERE node_id = ?
        ORDER BY timestamp DESC
        LIMIT ?
        """,
        (node_id, limit)
    )
    
    results = cursor.fetchall()
    conn.close()
    
    return [dict(row) for row in results]


@app.post("/api/control")
async def send_control_command(command: ControlCommand, current_user: User = Depends(check_permission("control"))):
    """Send a control command to a node via MQTT"""
    try:
        payload = {
            "action": command.action,
            "value": command.value,
            "timestamp": datetime.now().isoformat()
        }
        
        # Publish to MQTT
        mqtt_client.publish(f"wot/control/{command.node_id}", json.dumps(payload))
        
        # Store in database
        store_control_command({
            "node_id": command.node_id,
            "action": command.action,
            "value": command.value
        })
        
        # Broadcast to WebSocket clients
        await manager.broadcast({
            "type": "control_command", 
            "data": {
                "node_id": command.node_id,
                "action": command.action,
                "value": command.value,
                "timestamp": datetime.now().isoformat()
            }
        })
        
        return {"status": "success", "message": "Control command sent"}
    except Exception as e:
        logger.error(f"Error sending control command: {e}")
        raise HTTPException(status_code=500, detail=str(e))


@app.get("/api/nodes")
async def get_nodes(current_user: User = Depends(get_current_active_user)):
    """Get a list of all nodes in the system"""
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()
    
    cursor.execute("SELECT DISTINCT node_id FROM sensor_data")
    results = cursor.fetchall()
    conn.close()
    
    return [dict(row) for row in results]


# For user management (admin only)
@app.get("/api/users")
async def get_users(current_user: User = Depends(check_permission("admin"))):
    """Get a list of all users (admin only)"""
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()
    
    cursor.execute("SELECT id, username, email, full_name, disabled, permissions FROM users")
    results = cursor.fetchall()
    conn.close()
    
    users = []
    for row in results:
        user_dict = dict(row)
        user_dict["permissions"] = user_dict["permissions"].split(",") if user_dict["permissions"] else []
        users.append(user_dict)
    
    return users


@app.put("/api/users/{username}/permissions")
async def update_user_permissions(
    username: str,
    permissions: List[str],
    current_user: User = Depends(check_permission("admin"))
):
    """Update user permissions (admin only)"""
    if username == "admin" and "admin" not in permissions:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Cannot remove admin permission from admin user",
        )
    
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    cursor.execute("SELECT COUNT(*) FROM users WHERE username = ?", (username,))
    if cursor.fetchone()[0] == 0:
        conn.close()
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="User not found",
        )
    
    permissions_str = ",".join(permissions)
    cursor.execute(
        "UPDATE users SET permissions = ? WHERE username = ?",
        (permissions_str, username)
    )
    
    conn.commit()
    conn.close()
    
    return {"message": "User permissions updated successfully"}


@app.put("/api/users/{username}/status")
async def update_user_status(
    username: str,
    disabled: bool,
    current_user: User = Depends(check_permission("admin"))
):
    """Enable or disable a user (admin only)"""
    if username == "admin" and disabled:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Cannot disable admin user",
        )
    
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    cursor.execute("SELECT COUNT(*) FROM users WHERE username = ?", (username,))
    if cursor.fetchone()[0] == 0:
        conn.close()
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="User not found",
        )
    
    cursor.execute(
        "UPDATE users SET disabled = ? WHERE username = ?",
        (disabled, username)
    )
    
    conn.commit()
    conn.close()
    
    return {"message": f"User {'disabled' if disabled else 'enabled'} successfully"}


# WebSocket endpoint
@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time updates"""
    # Get token from query params
    # token = websocket.query_params.get("token")
    # if not token:
    #     await websocket.close(code=status.WS_1008_POLICY_VIOLATION)
    #     return
    
    # # Validate token
    # try:
    #     payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
    #     username = payload.get("sub")
    #     if not username:
    #         await websocket.close(code=status.WS_1008_POLICY_VIOLATION)
    #         return
    # except JWTError:
    #     await websocket.close(code=status.WS_1008_POLICY_VIOLATION)
    #     return
    username = "guest"
    
    # Connect to WebSocket
    await manager.connect(websocket, username)
    
    try:
        while True:
            # Keep the connection alive
            data = await websocket.receive_text()
            try:
                command = json.loads(data)
                if command.get("type") == "control":
                    # Validate user permissions for control commands
                    try:
                        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
                        permissions = payload.get("permissions", [])
                        if "admin" not in permissions and "control" not in permissions:
                            await websocket.send_json({
                                "type": "error",
                                "message": "Insufficient permissions for control commands"
                            })
                            continue
                    except JWTError:
                        await websocket.send_json({
                            "type": "error",
                            "message": "Invalid token"
                        })
                        continue
                    
                    # Handle control command sent via WebSocket
                    ctrl_cmd = ControlCommand(
                        node_id=command["data"]["node_id"],
                        action=command["data"]["action"],
                        value=command["data"]["value"]
                    )
                    
                    # Use a fake request context to call the API endpoint
                    response = await send_control_command(ctrl_cmd, get_user(username))
                    await websocket.send_json({
                        "type": "control_response",
                        "data": response
                    })
            except json.JSONDecodeError:
                pass
            except Exception as e:
                logger.error(f"Error processing WebSocket command: {e}")
                await websocket.send_json({
                    "type": "error",
                    "message": str(e)
                })
    except WebSocketDisconnect:
        manager.disconnect(websocket, username)


# Middleware to redirect unauthenticated users to login page for web routes
@app.middleware("http")
async def redirect_unauthenticated(request: Request, call_next):
    # Skip API routes and static files
    path = request.url.path
    if path.startswith("/api/") or path.startswith("/css/") or path.startswith("/js/") or path == "/login.html":
        response = await call_next(request)
        return response
    
    # Check if user is authenticated for web routes
    if path == "/" or path == "/index.html":
        token = request.cookies.get("access_token")
        if not token:
            # Redirect to login page
            return RedirectResponse(url="/login.html")
        
        try:
            # Validate token
            jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        except JWTError:
            # Token is invalid, redirect to login
            return RedirectResponse(url="/login.html")
    
    response = await call_next(request)
    return response


# Serve static files (frontend)
app.mount("/", StaticFiles(directory="static", html=True), name="static")


# Startup and shutdown events
@app.on_event("startup")
async def startup_event():
    """Initialize components on application startup"""
    # Initialize database
    init_db()
    
    # Connect to MQTT broker
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        logger.info(f"Started MQTT client, connecting to {MQTT_BROKER}:{MQTT_PORT}")
    except Exception as e:
        logger.error(f"Failed to connect to MQTT broker: {e}")


@app.on_event("shutdown")
async def shutdown_event():
    """Clean up resources on application shutdown"""
    # Disconnect MQTT client
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
    logger.info("Disconnected from MQTT broker")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("app:app", host="0.0.0.0", port=8000, reload=True)