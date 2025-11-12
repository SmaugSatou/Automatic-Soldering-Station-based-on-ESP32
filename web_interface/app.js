/**
 * @file app.js
 * @brief Main application logic for web interface
 * 
 * Handles G-Code upload, validation, visualization, WebSocket communication,
 * and UI interactions for soldering station control.
 */

// DOM elements
let fileInput;
let uploadBtn;
let fileNameDisplay;
let uploadStatus;
let selectedFile = null;

// Preview elements
let previewSection;
let drillContentDisplay;
let gcodeContentDisplay;
let sendBtn;
let cancelBtn;
let sendStatus;

// Control elements
let startBtn;
let pauseBtn;
let resumeBtn;
let stopBtn;
let controlStatus;

// Parsed data storage
let parsedGCode = null;
let originalDrillContent = null;

// Soldering parameters (configurable)
const SOLDERING_CONFIG = {
    temperature: 350,           // Soldering temperature in Celsius
    safeHeight: 10.0,          // Safe Z height for rapid moves (mm)
    approachHeight: 0.5,       // Height to slow down approach (mm)
    contactHeight: 0.1,        // Height for soldering contact (mm)
    preheatTime: 1000,         // Pad preheat time (ms)
    solderFeedAmount: 75,      // Solder wire feed steps
    flowTime: 800,             // Time to let solder flow (ms)
    approachFeedRate: 100,     // Feed rate for approach (mm/min)
    contactFeedRate: 50        // Feed rate for lowering to contact (mm/min)
};

/**
 * Initialize the application
 */
document.addEventListener('DOMContentLoaded', function() {
    // Get DOM elements
    fileInput = document.getElementById('drill-file-input');
    uploadBtn = document.getElementById('upload-btn');
    fileNameDisplay = document.getElementById('file-name');
    uploadStatus = document.getElementById('upload-status');
    
    // Preview elements
    previewSection = document.getElementById('preview-section');
    drillContentDisplay = document.getElementById('drill-content');
    gcodeContentDisplay = document.getElementById('gcode-content');
    sendBtn = document.getElementById('send-btn');
    cancelBtn = document.getElementById('cancel-btn');
    sendStatus = document.getElementById('send-status');
    
    // Control elements
    startBtn = document.getElementById('start-btn');
    pauseBtn = document.getElementById('pause-btn');
    resumeBtn = document.getElementById('resume-btn');
    stopBtn = document.getElementById('stop-btn');
    controlStatus = document.getElementById('control-status');
    
    uploadBtn.disabled = true;

    // Add event listeners
    if (fileInput) {
        fileInput.addEventListener('change', handleFileSelect);
    }
    
    if (uploadBtn) {
        uploadBtn.addEventListener('click', handleFileParse);
    }
    
    if (sendBtn) {
        sendBtn.addEventListener('click', handleSendToController);
    }
    
    if (cancelBtn) {
        cancelBtn.addEventListener('click', handleCancel);
    }
    
    if (startBtn) {
        startBtn.addEventListener('click', handleStart);
    }
    
    if (pauseBtn) {
        pauseBtn.addEventListener('click', handlePause);
    }
    
    if (resumeBtn) {
        resumeBtn.addEventListener('click', handleResume);
    }
    
    if (stopBtn) {
        stopBtn.addEventListener('click', handleStop);
    }
});

/**
 * Handle file selection
 */
function handleFileSelect(event) {
    const file = event.target.files[0];
    
    if (!file) {
        selectedFile = null;
        uploadBtn.disabled = true;
        fileNameDisplay.textContent = '';
        hidePreview();
        return;
    }

    // Check file extension
    if (!file.name.endsWith('.DRL')) {
        uploadStatus.textContent = 'Error: Only .DRL files are accepted';
        uploadStatus.className = 'upload-status error';
        selectedFile = null;
        uploadBtn.disabled = true;
        fileNameDisplay.textContent = '';
        hidePreview();
        return;
    }

    // Valid file selected
    selectedFile = file;
    fileNameDisplay.textContent = file.name;
    uploadBtn.disabled = false;
    uploadBtn.textContent = 'Parse';
    uploadStatus.textContent = '';
    uploadStatus.className = 'upload-status';
    hidePreview();
}

/**
 * Handle file parsing (first button click)
 */
async function handleFileParse() {
    if (!selectedFile) {
        return;
    }

    uploadStatus.textContent = 'Processing drill file...';
    uploadStatus.className = 'upload-status info';
    uploadBtn.disabled = true;

    try {
        // Read file content
        const fileContent = await readFileContent(selectedFile);
        originalDrillContent = fileContent;
        
        // Parse drill file to G-Code
        uploadStatus.textContent = 'Converting to G-Code...';
        const gcode = parseDrillToGCode(fileContent);
        
        if (!gcode) {
            throw new Error('Failed to parse drill file');
        }
        
        parsedGCode = gcode;
        
        // Display G-Code statistics
        const lines = gcode.split('\n').filter(l => l.trim() && !l.trim().startsWith(';'));
        uploadStatus.textContent = `Successfully parsed! Generated ${lines.length} G-Code commands.`;
        uploadStatus.className = 'upload-status success';
        
        // Show preview
        showPreview(fileContent, gcode);
        
    } catch (error) {
        uploadStatus.textContent = `Error: ${error.message}`;
        uploadStatus.className = 'upload-status error';
        uploadBtn.disabled = false;
    }
}

/**
 * Handle sending G-Code to controller (second button click)
 */
async function handleSendToController() {
    if (!parsedGCode || !selectedFile) {
        return;
    }

    sendStatus.textContent = 'Uploading to controller...';
    sendStatus.className = 'upload-status info';
    sendBtn.disabled = true;

    try {
        // Strip comments and empty lines for ESP32
        const cleanGCode = parsedGCode
            .split('\n')
            .map(line => line.trim())
            .filter(line => line && !line.startsWith(';'))  // Remove empty lines and comments
            .join('\n');
        
        // Send clean G-Code to server
        const response = await fetch('/api/gcode/upload', {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain',
            },
            body: cleanGCode
        });

        if (response.ok) {
            const result = await response.json();
            const commandCount = cleanGCode.split('\n').length;
            sendStatus.textContent = `Success: G-Code uploaded to controller (${commandCount} commands)`;
            sendStatus.className = 'upload-status success';
            
            // Enable start button
            if (startBtn) {
                startBtn.disabled = false;
            }
            
            // Reset after successful upload
            setTimeout(() => {
                resetUploadForm();
            }, 3000);
            
        } else {
            const error = await response.json();
            sendStatus.textContent = `Error: ${error.message || 'Upload failed'}`;
            sendStatus.className = 'upload-status error';
            sendBtn.disabled = false;
        }
    } catch (error) {
        sendStatus.textContent = `Error: ${error.message}`;
        sendStatus.className = 'upload-status error';
        sendBtn.disabled = false;
    }
}

/**
 * Handle start button click
 */
async function handleStart() {
    controlStatus.textContent = 'Starting execution...';
    controlStatus.className = 'upload-status info';
    
    try {
        const response = await fetch('/api/gcode/start', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });

        if (response.ok) {
            const result = await response.json();
            controlStatus.textContent = 'Execution started';
            controlStatus.className = 'upload-status success';
            
            // Update button states
            startBtn.disabled = true;
            pauseBtn.disabled = false;
            resumeBtn.disabled = true;
            stopBtn.disabled = false;
        } else {
            const error = await response.json();
            controlStatus.textContent = `Error: ${error.message || 'Failed to start'}`;
            controlStatus.className = 'upload-status error';
        }
    } catch (error) {
        controlStatus.textContent = `Error: ${error.message}`;
        controlStatus.className = 'upload-status error';
    }
}

/**
 * Handle pause button click
 */
async function handlePause() {
    controlStatus.textContent = 'Pausing execution...';
    controlStatus.className = 'upload-status info';
    
    try {
        const response = await fetch('/api/gcode/pause', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });

        if (response.ok) {
            const result = await response.json();
            controlStatus.textContent = 'Execution paused';
            controlStatus.className = 'upload-status success';
            
            // Update button states
            pauseBtn.disabled = true;
            resumeBtn.disabled = false;
        } else {
            const error = await response.json();
            controlStatus.textContent = `Error: ${error.message || 'Failed to pause'}`;
            controlStatus.className = 'upload-status error';
        }
    } catch (error) {
        controlStatus.textContent = `Error: ${error.message}`;
        controlStatus.className = 'upload-status error';
    }
}

/**
 * Handle resume button click
 */
async function handleResume() {
    controlStatus.textContent = 'Resuming execution...';
    controlStatus.className = 'upload-status info';
    
    try {
        const response = await fetch('/api/gcode/resume', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });

        if (response.ok) {
            const result = await response.json();
            controlStatus.textContent = 'Execution resumed';
            controlStatus.className = 'upload-status success';
            
            // Update button states
            pauseBtn.disabled = false;
            resumeBtn.disabled = true;
        } else {
            const error = await response.json();
            controlStatus.textContent = `Error: ${error.message || 'Failed to resume'}`;
            controlStatus.className = 'upload-status error';
        }
    } catch (error) {
        controlStatus.textContent = `Error: ${error.message}`;
        controlStatus.className = 'upload-status error';
    }
}

/**
 * Handle stop button click
 */
async function handleStop() {
    controlStatus.textContent = 'Stopping execution...';
    controlStatus.className = 'upload-status info';
    
    try {
        const response = await fetch('/api/gcode/stop', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });

        if (response.ok) {
            const result = await response.json();
            controlStatus.textContent = 'Execution stopped';
            controlStatus.className = 'upload-status success';
            
            // Reset button states
            startBtn.disabled = false;
            pauseBtn.disabled = true;
            resumeBtn.disabled = true;
            stopBtn.disabled = true;
            
            setTimeout(() => {
                controlStatus.textContent = '';
            }, 3000);
        } else {
            const error = await response.json();
            controlStatus.textContent = `Error: ${error.message || 'Failed to stop'}`;
            controlStatus.className = 'upload-status error';
        }
    } catch (error) {
        controlStatus.textContent = `Error: ${error.message}`;
        controlStatus.className = 'upload-status error';
    }
}

/**
 * Handle cancel button
 */
function handleCancel() {
    resetUploadForm();
}

/**
 * Show preview section with content
 */
function showPreview(drillContent, gcode) {
    drillContentDisplay.textContent = drillContent;
    gcodeContentDisplay.textContent = gcode;
    previewSection.style.display = 'block';
    sendStatus.textContent = '';
    sendStatus.className = 'upload-status';
    sendBtn.disabled = false;
    
    // Scroll to preview
    previewSection.scrollIntoView({ behavior: 'smooth', block: 'start' });
}

/**
 * Hide preview section
 */
function hidePreview() {
    previewSection.style.display = 'none';
    parsedGCode = null;
    originalDrillContent = null;
}

/**
 * Reset upload form to initial state
 */
function resetUploadForm() {
    fileInput.value = '';
    fileNameDisplay.textContent = '';
    selectedFile = null;
    uploadBtn.disabled = true;
    uploadBtn.textContent = 'Parse';
    uploadStatus.textContent = '';
    uploadStatus.className = 'upload-status';
    hidePreview();
    
    // Reset control buttons
    if (startBtn) startBtn.disabled = true;
    if (pauseBtn) pauseBtn.disabled = true;
    if (resumeBtn) resumeBtn.disabled = true;
    if (stopBtn) stopBtn.disabled = true;
    if (controlStatus) {
        controlStatus.textContent = '';
        controlStatus.className = 'upload-status';
    }
}

/**
 * Read file content as text
 */
function readFileContent(file) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = (e) => resolve(e.target.result);
        reader.onerror = (e) => reject(new Error('Failed to read file'));
        reader.readAsText(file);
    });
}

/**
 * Parse Excellon drill file to G-Code
 * @param {string} drillContent - Raw drill file content
 * @returns {string} Generated G-Code
 */
function parseDrillToGCode(drillContent) {
    const lines = drillContent.split('\n');
    let gcode = [];
    let currentTool = null;
    let toolSizes = {};
    let drillPoints = [];
    let inHeader = true;
    let isMetric = true;
    
    // Add header comments
    gcode.push('; G-Code generated from Excellon drill file');
    gcode.push('; Generated: ' + new Date().toISOString());
    gcode.push('; Soldering Station Automatic Controller');
    gcode.push('');
    
    // Parse drill file
    for (let line of lines) {
        line = line.trim();
        
        // Skip empty lines and comments
        if (!line || line.startsWith(';')) continue;
        
        // Check for metric/inch mode
        if (line.includes('METRIC')) {
            isMetric = true;
            continue;
        }
        if (line.includes('INCH')) {
            isMetric = false;
            continue;
        }
        
        // Parse tool definitions (TxxCxx.xxx)
        const toolMatch = line.match(/^T(\d+)C([\d.]+)/);
        if (toolMatch) {
            const toolNum = toolMatch[1];
            const toolSize = parseFloat(toolMatch[2]);
            toolSizes[toolNum] = toolSize;
            gcode.push(`; Tool T${toolNum}: ${toolSize}mm diameter`);
            continue;
        }
        
        // End of header
        if (line === '%' || line === 'G05' || line === 'G90') {
            if (inHeader) {
                inHeader = false;
                gcode.push('');
                gcode.push('; --- End of tool definitions ---');
                gcode.push('');
            }
            continue;
        }
        
        // Tool change (Txx alone on line)
        const toolChangeMatch = line.match(/^T(\d+)$/);
        if (toolChangeMatch) {
            currentTool = toolChangeMatch[1];
            const toolSize = toolSizes[currentTool] || 'unknown';
            gcode.push('');
            gcode.push(`; === Tool Change: T${currentTool} (${toolSize}mm) ===`);
            continue;
        }
        
        // Parse coordinates (X, Y with optional G85 for slots)
        const coordMatch = line.match(/X([-\d.]+)Y([-\d.]+)/);
        if (coordMatch && currentTool) {
            let x = parseFloat(coordMatch[1]);
            let y = parseFloat(coordMatch[2]);
            
            // Convert to mm if needed (though Excellon usually in mm already)
            if (!isMetric) {
                x *= 25.4;  // inches to mm
                y *= 25.4;
            }
            
            // Check for slot (G85)
            const isSlot = line.includes('G85');
            
            drillPoints.push({
                x: x,
                y: y,
                tool: currentTool,
                size: toolSizes[currentTool],
                isSlot: isSlot
            });
        }
    }
    
    // Generate G-Code for soldering operations
    gcode.push('');
    gcode.push('; === Initialization ===');
    gcode.push(`M109 S${SOLDERING_CONFIG.temperature}  ; Set temperature and wait`);
    gcode.push('G28                    ; Home all axes');
    gcode.push('');
    
    // Process each drill point as a solder point
    gcode.push(`; === Soldering Operations (${drillPoints.length} points) ===`);
    
    for (let i = 0; i < drillPoints.length; i++) {
        const point = drillPoints[i];
        
        gcode.push('');
        gcode.push(`; Point ${i + 1}/${drillPoints.length}: Tool T${point.tool} (${point.size}mm) at X${point.x.toFixed(2)} Y${point.y.toFixed(2)}`);
        
        // Move to position with safe height
        gcode.push(`G0 X${point.x.toFixed(3)} Y${point.y.toFixed(3)} Z${SOLDERING_CONFIG.safeHeight.toFixed(1)}`);
        
        // Lower to approach height
        gcode.push(`G0 Z${SOLDERING_CONFIG.approachHeight} F${SOLDERING_CONFIG.approachFeedRate}`);
        
        // Lower slowly to contact height
        gcode.push(`G1 Z${SOLDERING_CONFIG.contactHeight} F${SOLDERING_CONFIG.contactFeedRate}`);
        
        // Preheat pad
        gcode.push(`G4 P${SOLDERING_CONFIG.preheatTime}  ; Preheat pad`);
        
        // Feed solder
        gcode.push(`S${SOLDERING_CONFIG.solderFeedAmount}              ; Feed solder`);
        
        // Let solder flow
        gcode.push(`G4 P${SOLDERING_CONFIG.flowTime}   ; Let solder flow`);
        
        // Retract to safe height
        gcode.push(`G0 Z${SOLDERING_CONFIG.safeHeight.toFixed(1)}     ; Retract`);
    }
    
    // Cleanup
    gcode.push('');
    gcode.push('; === Cleanup ===');
    gcode.push('G28                    ; Return home');
    gcode.push('M104 S0                ; Turn off heater');
    gcode.push('');
    gcode.push('; === End of Program ===');
    
    return gcode.join('\n');
}

