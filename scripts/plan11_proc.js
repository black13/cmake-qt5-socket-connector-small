/**
 * Plan 11 - Runtime Extensible Node Actions
 * Processor Node Behaviors in JavaScript
 * 
 * This file demonstrates runtime-extensible node behaviors using
 * the rubber types action system. Actions are registered dynamically
 * and can be executed on nodes without modifying core classes.
 */

// ===============================================
// Action 1: Uppercase Text Payload
// ===============================================

/**
 * Converts input text to uppercase
 * Applicable to: PROC nodes with text input/output
 */
function uppercase_payload(node, ctx) {
    console.log("Action: uppercase_payload executing on node " + ctx.getNodeId());
    
    try {
        // Get the input text
        if (!ctx.hasInput("text")) {
            ctx.setError("uppercase_payload requires 'text' input");
            return;
        }
        
        let inputText = ctx.getInput("text");
        if (typeof inputText !== 'string') {
            inputText = String(inputText);
        }
        
        // Process: convert to uppercase
        let result = inputText.toUpperCase();
        
        // Set the output
        ctx.setOutput("result", result);
        
        console.log("uppercase_payload: '" + inputText + "' -> '" + result + "'");
        
    } catch (error) {
        ctx.setError("uppercase_payload failed: " + error.message);
    }
}

// ===============================================
// Action 2: Concatenate Multiple Inputs
// ===============================================

/**
 * Concatenates multiple text inputs with a separator
 * Applicable to: PROC nodes with multiple text inputs
 */
function concat_inputs(node, ctx) {
    console.log("Action: concat_inputs executing on node " + ctx.getNodeId());
    
    try {
        let inputs = [];
        let separator = " ";
        
        // Check for separator parameter
        if (ctx.hasInput("separator")) {
            separator = ctx.getInput("separator").toString();
        }
        
        // Collect all text inputs (input_0, input_1, input_2, ...)
        for (let i = 0; i < 10; i++) { // Check up to 10 inputs
            let inputName = "input_" + i;
            if (ctx.hasInput(inputName)) {
                let value = ctx.getInput(inputName);
                if (value !== null && value !== undefined) {
                    inputs.push(String(value));
                }
            }
        }
        
        // Also check for named inputs
        let namedInputs = ["text", "text1", "text2", "text3", "left", "right"];
        for (let name of namedInputs) {
            if (ctx.hasInput(name)) {
                let value = ctx.getInput(name);
                if (value !== null && value !== undefined) {
                    inputs.push(String(value));
                }
            }
        }
        
        if (inputs.length === 0) {
            ctx.setError("concat_inputs: No valid inputs found");
            return;
        }
        
        // Concatenate with separator
        let result = inputs.join(separator);
        ctx.setOutput("result", result);
        
        console.log("concat_inputs: [" + inputs.join(", ") + "] -> '" + result + "'");
        
    } catch (error) {
        ctx.setError("concat_inputs failed: " + error.message);
    }
}

// ===============================================
// Action 3: Hash Input Data
// ===============================================

/**
 * Generates a simple hash of input data
 * Applicable to: PROC nodes for data integrity checks
 */
function hash_input(node, ctx) {
    console.log("Action: hash_input executing on node " + ctx.getNodeId());
    
    try {
        // Get all available inputs
        let inputData = "";
        let inputCount = 0;
        
        // Collect all inputs into a string for hashing
        for (let i = 0; i < 10; i++) {
            let inputName = "input_" + i;
            if (ctx.hasInput(inputName)) {
                let value = ctx.getInput(inputName);
                if (value !== null && value !== undefined) {
                    inputData += String(value);
                    inputCount++;
                }
            }
        }
        
        // Also check common named inputs
        let namedInputs = ["text", "data", "payload", "value"];
        for (let name of namedInputs) {
            if (ctx.hasInput(name)) {
                let value = ctx.getInput(name);
                if (value !== null && value !== undefined) {
                    inputData += String(value);
                    inputCount++;
                }
            }
        }
        
        if (inputCount === 0) {
            ctx.setError("hash_input: No inputs to hash");
            return;
        }
        
        // Generate simple hash (djb2 algorithm)
        let hash = 5381;
        for (let i = 0; i < inputData.length; i++) {
            hash = ((hash << 5) + hash) + inputData.charCodeAt(i);
            hash = hash & 0xFFFFFFFF; // Keep it 32-bit
        }
        
        let hashHex = (hash >>> 0).toString(16).padStart(8, '0');
        
        // Output both numeric and hex formats
        ctx.setOutput("hash_numeric", hash >>> 0);
        ctx.setOutput("hash_hex", hashHex);
        ctx.setOutput("result", hashHex); // Default output
        
        console.log("hash_input: " + inputCount + " inputs, data length: " + 
                   inputData.length + ", hash: " + hashHex);
        
    } catch (error) {
        ctx.setError("hash_input failed: " + error.message);
    }
}

// ===============================================
// Action 4: JSON Transform
// ===============================================

/**
 * Transforms JSON input by extracting or modifying fields
 * Applicable to: PROC nodes working with structured data
 */
function json_transform(node, ctx) {
    console.log("Action: json_transform executing on node " + ctx.getNodeId());
    
    try {
        if (!ctx.hasInput("json")) {
            ctx.setError("json_transform requires 'json' input");
            return;
        }
        
        let jsonInput = ctx.getInput("json");
        let jsonObj;
        
        // Parse JSON if it's a string
        if (typeof jsonInput === 'string') {
            try {
                jsonObj = JSON.parse(jsonInput);
            } catch (parseError) {
                ctx.setError("json_transform: Invalid JSON input - " + parseError.message);
                return;
            }
        } else if (typeof jsonInput === 'object') {
            jsonObj = jsonInput;
        } else {
            ctx.setError("json_transform: Input must be JSON string or object");
            return;
        }
        
        // Get transform operation (default: extract all)
        let operation = "extract_all";
        if (ctx.hasInput("operation")) {
            operation = ctx.getInput("operation").toString();
        }
        
        let result = {};
        
        switch (operation) {
            case "extract_all":
                result = jsonObj;
                break;
                
            case "keys_only":
                if (typeof jsonObj === 'object' && jsonObj !== null) {
                    result = { keys: Object.keys(jsonObj) };
                } else {
                    result = { keys: [] };
                }
                break;
                
            case "flatten":
                result = flattenObject(jsonObj);
                break;
                
            case "count_fields":
                if (typeof jsonObj === 'object' && jsonObj !== null) {
                    result = { field_count: Object.keys(jsonObj).length };
                } else {
                    result = { field_count: 0 };
                }
                break;
                
            default:
                ctx.setError("json_transform: Unknown operation '" + operation + "'");
                return;
        }
        
        ctx.setOutput("result", JSON.stringify(result));
        ctx.setOutput("json_object", result);
        
        console.log("json_transform: Operation '" + operation + "' completed");
        
    } catch (error) {
        ctx.setError("json_transform failed: " + error.message);
    }
}

/**
 * Helper function to flatten nested JSON objects
 */
function flattenObject(obj, prefix = '') {
    let flattened = {};
    
    for (let key in obj) {
        if (obj.hasOwnProperty(key)) {
            let newKey = prefix ? prefix + '.' + key : key;
            
            if (typeof obj[key] === 'object' && obj[key] !== null && !Array.isArray(obj[key])) {
                Object.assign(flattened, flattenObject(obj[key], newKey));
            } else {
                flattened[newKey] = obj[key];
            }
        }
    }
    
    return flattened;
}

// ===============================================
// Action Registration
// ===============================================

/**
 * Register all actions with the ActionRegistry
 * This function would be called by the JavaScript engine integration
 */
function registerPlan11Actions() {
    console.log("Registering Plan 11 processor actions...");
    
    // Register with the ActionRegistry (pseudo-code, actual implementation
    // would depend on how JavaScript integrates with the C++ ActionRegistry)
    
    if (typeof registerNodeAction === 'function') {
        registerNodeAction("PROC", "uppercase_payload", uppercase_payload);
        registerNodeAction("PROC", "concat_inputs", concat_inputs);
        registerNodeAction("PROC", "hash_input", hash_input);
        registerNodeAction("PROC", "json_transform", json_transform);
        
        console.log("Plan 11 actions registered successfully");
    } else {
        console.log("registerNodeAction not available - actions defined but not registered");
    }
}

// ===============================================
// Action Metadata and Documentation
// ===============================================

/**
 * Get metadata about available actions
 */
function getActionMetadata() {
    return {
        "uppercase_payload": {
            description: "Converts input text to uppercase",
            inputs: ["text"],
            outputs: ["result"],
            nodeTypes: ["PROC"]
        },
        "concat_inputs": {
            description: "Concatenates multiple text inputs with separator",
            inputs: ["input_0", "input_1", "...", "separator"],
            outputs: ["result"],
            nodeTypes: ["PROC"]
        },
        "hash_input": {
            description: "Generates hash of input data for integrity checks",
            inputs: ["input_0", "input_1", "...", "text", "data"],
            outputs: ["result", "hash_numeric", "hash_hex"],
            nodeTypes: ["PROC"]
        },
        "json_transform": {
            description: "Transforms JSON data with various operations",
            inputs: ["json", "operation"],
            outputs: ["result", "json_object"],
            nodeTypes: ["PROC"]
        }
    };
}

// Auto-register if the registration function is available
if (typeof registerPlan11Actions === 'function') {
    registerPlan11Actions();
}

console.log("Plan 11 processor actions loaded");