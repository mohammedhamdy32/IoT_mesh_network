# Distributed Web of Things (WoT) System

This project implements a complete web solution for a Distributed Web of Things system that connects ESP32 mesh nodes via MQTT to a FastAPI backend with a responsive frontend dashboard for real-time monitoring and control.

## Project Structure

```
wot-system/
├── app.py                  # FastAPI backend application
├── requirements.txt        # Python dependencies
├── static/                 # Frontend static files
│   ├── index.html          # Main HTML file
│   ├── css/
│   │   └── style.css       # CSS styles
│   └── js/
│       └── app.js          # Frontend JavaScript
└── README.md               # Project documentation
```

## Features

### Backend (FastAPI)

- MQTT integration for subscribing to sensor data and publishing control commands
- SQLite database for storing sensor data and control commands
- REST API endpoints for retrieving latest data and historical information
- WebSocket support for real-time updates to the frontend
- Error handling and logging

### Frontend (HTML, CSS, JavaScript)

- Responsive dashboard compatible with desktop and mobile devices
- Real-time data visualization with Chart.js
- Interactive controls for sending commands to nodes
- Activity log for tracking sensor data and control commands
- Support for multiple sensor nodes

## Setup Instructions

### Prerequisites

- Python 3.8+
- MQTT broker (like Mosquitto) running and accessible
- ESP32 mesh nodes publishing to MQTT topics

### Installation

1. Clone the repository (Folder)

2. Create a virtual environment and activate it:

```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
```

3. Install dependencies:

```bash
pip install -r requirements.txt
```

4. Configure MQTT broker settings:
   Edit `app.py` and update the following variables if needed:

```python
MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
```

Alternatively, set environment variables:

```bash
export MQTT_BROKER=your_broker_address
export MQTT_PORT=1883
```

5. Run the application:

```bash
uvicorn app:app --host 0.0.0.0 --port 8000 --reload
```

6. Access the dashboard at http://localhost:8000

7. Create your own user account (read sensor data permission only)

8. For admin access (read sensor data and control commands), login with:

   - Username: `admin`
   - Password: `admin123`


## MQTT Topic Structure

- **Sensor Data**: `wot/sensors/<node_id>`

  - Example payload: `{"temperature": 25.4, "humidity": 65.2, "light": 350, "pressure": 1013.2}`

- **Control Commands**: `wot/control/<node_id>`
  - Example payload: `{"action": "servo", "value": 90}`

## API Endpoints

- `GET /api/latest` - Get the latest sensor readings for all nodes
- `GET /api/history/{node_id}` - Get historical sensor data for a specific node
- `GET /api/nodes` - Get a list of all nodes in the system
- `POST /api/control` - Send a control command to a node
- `WebSocket /ws` - Connect for real-time updates

## Development

### Required Python packages

Create a `requirements.txt` file with the following dependencies:

```
fastapi>=0.95.0
uvicorn>=0.21.1
paho-mqtt>=2.1.1
pydantic>=1.10.7
websockets>=11.0.2
sqlalchemy>=2.0.9
aiohttp>=3.8.4
```

### ESP32 Node Integration

For the ESP32 mesh nodes to integrate with this system, they should:

1. Connect to the MQTT broker
2. Publish sensor data to topics in the format `wot/sensors/<node_id>`
3. Subscribe to control commands on topics in the format `wot/control/<node_id>`
4. Process received control commands (e.g., actuate servos or control LEDs)

Example ESP32 Arduino code for a node would handle:

- Reading sensors
- Publishing data to MQTT
- Subscribing to control commands
