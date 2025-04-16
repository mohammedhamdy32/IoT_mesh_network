// Configuration
const API_BASE_URL = window.location.protocol + '//' + window.location.host;
const WS_BASE_URL = window.location.protocol === 'https:' ? 'wss://' : 'ws://' + window.location.host;
const WS_ENDPOINT = `${WS_BASE_URL}/ws`;
const MAX_DATA_POINTS = 20;
const REFRESH_INTERVAL = 10000; // 10 seconds

// Global state
const state = {
    socket: null,
    connected: false,
    activeNode: null,
    nodes: {},
    charts: {},
    dataHistory: {},
    logEntries: []
};

// DOM Elements
const connectionStatus = document.getElementById('connection-status');
const nodeList = document.getElementById('node-list');
const overviewCards = document.getElementById('overview-cards');
const nodeSelector = document.getElementById('node-selector');
const actionType = document.getElementById('action-type');
const controlValue = document.getElementById('control-value');
const valueDisplay = document.getElementById('value-display');
const controlForm = document.getElementById('control-form');
const activityLog = document.getElementById('activity-log');
const btnRealtime = document.getElementById('btn-realtime');
const btnHistory = document.getElementById('btn-history');
const notificationToast = document.getElementById('notification-toast');
const toastTitle = document.getElementById('toast-title');
const toastTime = document.getElementById('toast-time');
const toastMessage = document.getElementById('toast-message');

// Toast notification instance
const toast = new bootstrap.Toast(notificationToast);

// Chart colors
const chartColors = {
    temperature: {
        border: 'rgb(255, 99, 132)',
        background: 'rgba(255, 99, 132, 0.2)'
    },
    humidity: {
        border: 'rgb(54, 162, 235)',
        background: 'rgba(54, 162, 235, 0.2)'
    },
    light: {
        border: 'rgb(255, 206, 86)',
        background: 'rgba(255, 206, 86, 0.2)'
    },
    pressure: {
        border: 'rgb(75, 192, 192)',
        background: 'rgba(75, 192, 192, 0.2)'
    }
};

// Sensor icons and units
const sensorInfo = {
    temperature: { icon: 'fa-temperature-half', unit: '°C', color: '#dc3545' },
    humidity: { icon: 'fa-droplet', unit: '%', color: '#0dcaf0' },
    light: { icon: 'fa-sun', unit: 'lux', color: '#ffc107' },
    pressure: { icon: 'fa-gauge-high', unit: 'hPa', color: '#6f42c1' }
};

// Initialize charts
function initCharts() {
    const chartOptions = {
        responsive: true,
        maintainAspectRatio: false,
        animation: {
            duration: 500
        },
        scales: {
            x: {
                type: 'time',
                time: {
                    unit: 'minute',
                    displayFormats: {
                        minute: 'HH:mm:ss'
                    }
                },
                title: {
                    display: true,
                    text: 'Time'
                }
            },
            y: {
                beginAtZero: false
            }
        },
        plugins: {
            legend: {
                display: true,
                position: 'top'
            }
        }
    };

    // Initialize all charts
    state.charts.temperature = new Chart(
        document.getElementById('temperatureChart'),
        {
            type: 'line',
            data: {
                datasets: []
            },
            options: {
                ...chartOptions,
                scales: {
                    ...chartOptions.scales,
                    y: {
                        ...chartOptions.scales.y,
                        title: {
                            display: true,
                            text: 'Temperature (°C)'
                        }
                    }
                },
                plugins: {
                    ...chartOptions.plugins,
                    title: {
                        display: true,
                        text: 'Temperature'
                    }
                }
            }
        }
    );


    state.charts.humidity = new Chart(
        document.getElementById('humidityChart'),
        {
            type: 'line',
            data: {
                datasets: []
            },
            options: {
                ...chartOptions,
                scales: {
                    ...chartOptions.scales,
                    y: {
                        ...chartOptions.scales.y,
                        title: {
                            display: true,
                            text: 'Humidity (%)'
                        },
                        min: 0,
                        max: 100
                    }
                },
                plugins: {
                    ...chartOptions.plugins,
                    title: {
                        display: true,
                        text: 'Humidity'
                    }
                }
            }
        }
    );

    state.charts.light = new Chart(
        document.getElementById('lightChart'),
        {
            type: 'line',
            data: {
                datasets: []
            },
            options: {
                ...chartOptions,
                scales: {
                    ...chartOptions.scales,
                    y: {
                        ...chartOptions.scales.y,
                        title: {
                            display: true,
                            text: 'Light (lux)'
                        },
                        min: 0
                    }
                },
                plugins: {
                    ...chartOptions.plugins,
                    title: {
                        display: true,
                        text: 'Light Intensity'
                    }
                }
            }
        }
    );

    state.charts.pressure = new Chart(
        document.getElementById('pressureChart'),
        {
            type: 'line',
            data: {
                datasets: []
            },
            options: {
                ...chartOptions,
                scales: {
                    ...chartOptions.scales,
                    y: {
                        ...chartOptions.scales.y,
                        title: {
                            display: true,
                            text: 'Pressure (hPa)'
                        }
                    }
                },
                plugins: {
                    ...chartOptions.plugins,
                    title: {
                        display: true,
                        text: 'Atmospheric Pressure'
                    }
                }
            }
        }
    );
}

// WebSocket connection
function connectWebSocket() {
    if (state.socket) {
        state.socket.close();
    }

    state.socket = new WebSocket(WS_ENDPOINT);

    state.socket.onopen = function() {
        state.connected = true;
        connectionStatus.textContent = 'Connected';
        connectionStatus.classList.remove('bg-danger');
        connectionStatus.classList.add('bg-success', 'connected');
        showNotification('Connection established', 'Successfully connected to the WebSocket server');
    };

    state.socket.onclose = function() {
        state.connected = false;
        connectionStatus.textContent = 'Disconnected';
        connectionStatus.classList.remove('bg-success', 'connected');
        connectionStatus.classList.add('bg-danger');
        
        // Try to reconnect after a delay
        setTimeout(connectWebSocket, 5000);
    };

    state.socket.onerror = function(error) {
        console.error('WebSocket error:', error);
        showNotification('Connection error', 'Error connecting to server. Retrying...', 'error');
    };

    state.socket.onmessage = function(event) {
        const message = JSON.parse(event.data);
        
        if (message.type === 'sensor_data') {
            processSensorData(message.data);
        } else if (message.type === 'control_command') {
            processControlCommand(message.data);
        }
    };
}

// Process incoming sensor data
function processSensorData(data) {
    const nodeId = data.node_id;
    const timestamp = new Date(data.timestamp);
    
    // Initialize node if it doesn't exist
    if (!state.nodes[nodeId]) {
        state.nodes[nodeId] = {
            id: nodeId,
            lastSeen: timestamp,
            online: true,
            data: {}
        };
        
        // Initialize data history for this node
        state.dataHistory[nodeId] = {
            temperature: [],
            humidity: [],
            light: [],
            pressure: []
        };
        
        // Update node list
        updateNodeList();
        updateNodeSelector();
    }
    
    // Update node data
    state.nodes[nodeId].lastSeen = timestamp;
    state.nodes[nodeId].online = true;
    
    // Update sensor data
    ['temperature', 'humidity', 'light', 'pressure'].forEach(sensor => {
        if (data[sensor] !== undefined && data[sensor] !== null) {
            state.nodes[nodeId].data[sensor] = data[sensor];
            
            // Add to history
            state.dataHistory[nodeId][sensor].push({
                x: timestamp,
                y: data[sensor]
            });
            
            // Limit history size
            if (state.dataHistory[nodeId][sensor].length > MAX_DATA_POINTS) {
                state.dataHistory[nodeId][sensor].shift();
            }
        }
    });
    
    // Update UI
    updateNodeStatus(nodeId);
    updateOverviewCards();
    updateCharts();
    
    // Add to activity log
    addLogEntry({
        timestamp: timestamp,
        nodeId: nodeId,
        type: 'sensor-data',
        details: formatSensorDetails(data)
    });
    
    // Show notification for active node
    if (state.activeNode === nodeId) {
        showNotification(
            `New data from ${nodeId}`,
            formatSensorDetails(data),
            'info'
        );
    }
}

// Process control command
function processControlCommand(data) {
    const nodeId = data.node_id;
    const timestamp = new Date(data.timestamp);
    
    // Add to activity log
    addLogEntry({
        timestamp: timestamp,
        nodeId: nodeId,
        type: 'control-command',
        details: `${data.action}: ${data.value}`
    });
    
    // Show notification
    showNotification(
        `Control command sent to ${nodeId}`,
        `${data.action}: ${data.value}`,
        'success'
    );
}

// Update node list in sidebar
function updateNodeList() {
    nodeList.innerHTML = '';
    
    if (Object.keys(state.nodes).length === 0) {
        nodeList.innerHTML = '<div class="text-center p-3"><p class="text-muted">No nodes found</p></div>';
        return;
    }
    
    // Sort nodes by ID
    const sortedNodes = Object.values(state.nodes).sort((a, b) => a.id.localeCompare(b.id));
    
    sortedNodes.forEach(node => {
        const nodeElement = document.createElement('div');
        nodeElement.className = `node-item ${state.activeNode === node.id ? 'active' : ''}`;
        nodeElement.dataset.nodeId = node.id;
        
        const statusClass = node.online ? 'online' : '';
        
        nodeElement.innerHTML = `
            <div class="status-indicator ${statusClass}"></div>
            <div class="node-info">
                <div class="node-name">${node.id}</div>
                <div class="node-status">${node.online ? 'Online' : 'Offline'}</div>
                <div class="node-last-seen">Last seen: ${formatTimestamp(node.lastSeen)}</div>
            </div>
        `;
        
        nodeElement.addEventListener('click', () => {
            setActiveNode(node.id);
        });
        
        nodeList.appendChild(nodeElement);
    });
}

// Update node selector in control panel
function updateNodeSelector() {
    // Save current selection
    const currentSelection = nodeSelector.value;
    
    // Clear options except the placeholder
    while (nodeSelector.options.length > 1) {
        nodeSelector.remove(1);
    }
    
    // Add node options
    const sortedNodes = Object.values(state.nodes).sort((a, b) => a.id.localeCompare(b.id));
    
    sortedNodes.forEach(node => {
        const option = document.createElement('option');
        option.value = node.id;
        option.textContent = node.id;
        nodeSelector.appendChild(option);
    });
    
    // Restore selection if possible, otherwise select the first node
    if (currentSelection && Array.from(nodeSelector.options).some(opt => opt.value === currentSelection)) {
        nodeSelector.value = currentSelection;
    } else if (sortedNodes.length > 0 && state.activeNode) {
        nodeSelector.value = state.activeNode;
    }
}

// Update node status
function updateNodeStatus(nodeId) {
    const nodeElements = document.querySelectorAll(`.node-item[data-node-id="${nodeId}"]`);
    
    nodeElements.forEach(element => {
        const node = state.nodes[nodeId];
        
        const statusIndicator = element.querySelector('.status-indicator');
        const nodeStatus = element.querySelector('.node-status');
        const nodeLastSeen = element.querySelector('.node-last-seen');
        
        statusIndicator.className = `status-indicator ${node.online ? 'online' : ''}`;
        nodeStatus.textContent = node.online ? 'Online' : 'Offline';
        nodeLastSeen.textContent = `Last seen: ${formatTimestamp(node.lastSeen)}`;
        
        // Add highlight animation
        element.classList.add('sensor-update');
        setTimeout(() => {
            element.classList.remove('sensor-update');
        }, 1000);
    });
}

// Set active node
function setActiveNode(nodeId) {
    state.activeNode = nodeId;
    
    // Update node list
    document.querySelectorAll('.node-item').forEach(element => {
        if (element.dataset.nodeId === nodeId) {
            element.classList.add('active');
        } else {
            element.classList.remove('active');
        }
    });
    
    // Update node selector
    nodeSelector.value = nodeId;
    
    // Update charts
    updateCharts();
    
    // Update overview cards
    updateOverviewCards();
}

// Update overview cards
function updateOverviewCards() {
    overviewCards.innerHTML = '';
    
    if (!state.activeNode || !state.nodes[state.activeNode]) {
        return;
    }
    
    const node = state.nodes[state.activeNode];
    
    // Create a card for each sensor type
    ['temperature', 'humidity', 'light', 'pressure'].forEach(sensor => {
        if (node.data[sensor] !== undefined) {
            const card = document.createElement('div');
            card.className = 'col-sm-6 col-lg-3 mb-3';
            
            const info = sensorInfo[sensor];
            
            card.innerHTML = `
                <div class="card overview-card">
                    <div class="card-body d-flex align-items-center">
                        <div class="card-icon text-${getColorClass(info.color)}">
                            <i class="fas ${info.icon}"></i>
                        </div>
                        <div>
                            <div class="card-value">${formatValue(node.data[sensor])}<span class="fs-6">${info.unit}</span></div>
                            <div class="card-label">${capitalizeFirstLetter(sensor)}</div>
                        </div>
                    </div>
                </div>
            `;
            
            overviewCards.appendChild(card);
        }
    });
}

// Update charts
function updateCharts() {
    // Clear all charts
    Object.values(state.charts).forEach(chart => {
        chart.data.datasets = [];
        chart.update();
    });
    
    if (!state.activeNode || !state.nodes[state.activeNode]) {
        return;
    }
    
    const nodeId = state.activeNode;
    
    // Update each chart with the node's data
    ['temperature', 'humidity', 'light', 'pressure'].forEach(sensor => {
        if (state.dataHistory[nodeId] && state.dataHistory[nodeId][sensor]) {
            const data = state.dataHistory[nodeId][sensor];
            
            if (data.length > 0) {
                state.charts[sensor].data.datasets = [{
                    label: `${capitalizeFirstLetter(sensor)} - ${nodeId}`,
                    data: data,
                    borderColor: chartColors[sensor].border,
                    backgroundColor: chartColors[sensor].background,
                    borderWidth: 2,
                    tension: 0.2,
                    pointRadius: 3,
                    pointHoverRadius: 5
                }];
                
                state.charts[sensor].update();

            }
        }
    });
}

// Add log entry
function addLogEntry(entry) {
    // Add to state
    state.logEntries.unshift(entry);
    
    // Limit log size
    if (state.logEntries.length > 100) {
        state.logEntries.pop();
    }
    
    // Update UI
    updateActivityLog();
}

// Update activity log
function updateActivityLog() {
    activityLog.innerHTML = '';
    
    if (state.logEntries.length === 0) {
        activityLog.innerHTML = '<tr><td colspan="4" class="text-center text-muted">No activity recorded yet</td></tr>';
        return;
    }
    
    state.logEntries.slice(0, 10).forEach(entry => {
        const row = document.createElement('tr');
        
        const badgeClass = entry.type === 'sensor-data' ? 'sensor-data' : 'control-command';
        
        row.innerHTML = `
            <td>${formatTimestamp(entry.timestamp)}</td>
            <td>${entry.nodeId}</td>
            <td><span class="data-badge ${badgeClass}">${entry.type === 'sensor-data' ? 'Data' : 'Control'}</span></td>
            <td>${entry.details}</td>
        `;
        
        activityLog.appendChild(row);
    });
}

// Format sensor details
function formatSensorDetails(data) {
    const details = [];
    
    ['temperature', 'humidity', 'light', 'pressure'].forEach(sensor => {
        if (data[sensor] !== undefined && data[sensor] !== null) {
            const info = sensorInfo[sensor];
            details.push(`${capitalizeFirstLetter(sensor)}: ${formatValue(data[sensor])}${info.unit}`);
        }
    });
    
    return details.join(', ');
}

// Format timestamp
function formatTimestamp(timestamp) {
    const date = new Date(timestamp);
    return date.toLocaleTimeString();
}

// Format value
function formatValue(value) {
    return typeof value === 'number' ? value.toFixed(1) : value;
}

// Capitalize first letter
function capitalizeFirstLetter(string) {
    return string.charAt(0).toUpperCase() + string.slice(1);
}

// Get Bootstrap color class from hex
function getColorClass(hexColor) {
    const colorMap = {
        '#dc3545': 'danger',
        '#0dcaf0': 'info',
        '#ffc107': 'warning',
        '#6f42c1': 'purple',
        '#0d6efd': 'primary',
        '#198754': 'success',
        '#6c757d': 'secondary'
    };
    
    return colorMap[hexColor] || 'primary';
}

// Show notification
function showNotification(title, message, type = 'info') {
    toastTitle.textContent = title;
    toastMessage.textContent = message;
    toastTime.textContent = new Date().toLocaleTimeString();
    
    // Set toast color based on type
    notificationToast.className = 'toast';
    if (type === 'error') {
        notificationToast.classList.add('border-danger');
    } else if (type === 'success') {
        notificationToast.classList.add('border-success');
    } else {
        notificationToast.classList.add('border-info');
    }
    
    toast.show();
}

// Fetch latest sensor data
async function fetchLatestData() {
    try {
        const accessToken = localStorage.getItem('access_token') || sessionStorage.getItem('access_token');
        const response = await fetch(`${API_BASE_URL}/api/latest`, {
        headers: {
            'Authorization': `Bearer ${accessToken}`,
        },
        });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        
        // Process each node's data
        data.forEach(nodeData => {
            const timestamp = new Date(nodeData.timestamp);
            
            processSensorData({
                node_id: nodeData.node_id,
                temperature: nodeData.temperature,
                humidity: nodeData.humidity,
                light: nodeData.light,
                pressure: nodeData.pressure,
                timestamp: timestamp
            });
        });
    } catch (error) {
        console.error('Error fetching latest data:', error);
        showNotification('Error', 'Failed to fetch the latest sensor data', 'error');
    }
}

// Fetch node history
async function fetchNodeHistory(nodeId) {
    try {
        const accessToken = localStorage.getItem('access_token') || sessionStorage.getItem('access_token');
        const response = await fetch(`${API_BASE_URL}/api/history/${nodeId}`, {
        headers: {
            'Authorization': `Bearer ${accessToken}`,
        },
        });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        
        // Clear history for this node
        state.dataHistory[nodeId] = {
            temperature: [],
            humidity: [],
            light: [],
            pressure: []
        };
        
        // Process history data (in reverse to get chronological order)
        data.reverse().forEach(entry => {
            const timestamp = new Date(entry.timestamp);
            
            ['temperature', 'humidity', 'light', 'pressure'].forEach(sensor => {
                if (entry[sensor] !== undefined && entry[sensor] !== null) {
                    state.dataHistory[nodeId][sensor].push({
                        x: timestamp,
                        y: entry[sensor]
                    });
                }
            });
        });
        
        // Update charts
        updateCharts();
        
        showNotification('History loaded', `Loaded historical data for ${nodeId}`, 'success');
    } catch (error) {
        console.error('Error fetching node history:', error);
        showNotification('Error', 'Failed to fetch node history', 'error');
    }
}

// Fetch available nodes
async function fetchNodes() {
    try {
        const accessToken = localStorage.getItem('access_token') || sessionStorage.getItem('access_token');
        const response = await fetch(`${API_BASE_URL}/api/nodes`, {
        headers: {
            'Authorization': `Bearer ${accessToken}`,
        },
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        
        // If we have nodes and no active node is set, set the first one
        if (data.length > 0 && !state.activeNode) {
            const firstNodeId = data[0].node_id;
            
            // Initialize node if it doesn't exist
            if (!state.nodes[firstNodeId]) {
                state.nodes[firstNodeId] = {
                    id: firstNodeId,
                    lastSeen: new Date(),
                    online: false,
                    data: {}
                };
                
                state.dataHistory[firstNodeId] = {
                    temperature: [],
                    humidity: [],
                    light: [],
                    pressure: []
                };
            }
            
            setActiveNode(firstNodeId);
        }
    } catch (error) {
        console.error('Error fetching nodes:', error);
        showNotification('Error', 'Failed to fetch available nodes', 'error');
    }
}

// Send control command
async function sendControlCommand(nodeId, action, value) {
    if (!state.connected) {
        showNotification('Error', 'Not connected to the server', 'error');
        return false;
    }
    
    try {
        const accessToken = localStorage.getItem('access_token') || sessionStorage.getItem('access_token');
        const response = await fetch(`${API_BASE_URL}/api/control`, {
            method: 'POST',
            headers: {
                'Authorization': `Bearer ${accessToken}`,
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                node_id: nodeId,
                action: action,
                value: parseFloat(value)
            })
        });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const result = await response.json();
        return true;
    } catch (error) {
        console.error('Error sending control command:', error);
        showNotification('Error', 'Failed to send control command', 'error');
        return false;
    }
}

// Event Listeners
document.addEventListener('DOMContentLoaded', () => {
    // Initialize charts
    initCharts();
    
    // Connect to WebSocket
    connectWebSocket();
    
    // Fetch initial data
    fetchNodes();
    fetchLatestData();
    
    // Control value slider event
    controlValue.addEventListener('input', () => {
        valueDisplay.textContent = controlValue.value;
    });
    
    // Control form submission
    controlForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const nodeId = nodeSelector.value;
        const action = actionType.value;
        const value = controlValue.value;
        
        if (!nodeId) {
            showNotification('Error', 'Please select a node', 'error');
            return;
        }
        
        const success = await sendControlCommand(nodeId, action, value);
        
        if (success) {
            showNotification('Success', `Sent ${action} command with value ${value} to ${nodeId}`, 'success');
        }
    });
    
    // Real-time / History toggle
    btnRealtime.addEventListener('click', () => {
        btnRealtime.classList.remove('btn-outline-light');
        btnRealtime.classList.add('btn-light');
        btnHistory.classList.remove('btn-light');
        btnHistory.classList.add('btn-outline-light');
        
        // Refresh with latest data
        if (state.activeNode) {
            fetchLatestData();
        }
    });
    
    btnHistory.addEventListener('click', () => {
        btnHistory.classList.remove('btn-outline-light');
        btnHistory.classList.add('btn-light');
        btnRealtime.classList.remove('btn-light');
        btnRealtime.classList.add('btn-outline-light');
        
        // Fetch history for active node
        if (state.activeNode) {
            fetchNodeHistory(state.activeNode);
        }
    });
    
    // Periodic refresh
    setInterval(() => {
        // Check node online status
        const now = new Date();
        
        Object.values(state.nodes).forEach(node => {
            const lastSeenTime = new Date(node.lastSeen).getTime();
            const timeDiff = now.getTime() - lastSeenTime;
            
            // If we haven't seen data for more than 30 seconds, mark as offline
            if (timeDiff > 30000 && node.online) {
                node.online = false;
                updateNodeStatus(node.id);
            }
        });
        
        // Refresh data in real-time mode
        if (btnRealtime.classList.contains('btn-light')) {
            fetchLatestData();
        }
    }, REFRESH_INTERVAL);
});
