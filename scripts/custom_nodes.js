// Custom Node Types in JavaScript
// Define custom node behaviors and processing logic

// Custom node type: Math Calculator
function mathCalculatorNode(inputs) {
    console.log("Math Calculator Node processing");
    
    const operation = inputs.operation || "add";
    const valueA = parseFloat(inputs.valueA) || 0;
    const valueB = parseFloat(inputs.valueB) || 0;
    
    let result;
    
    switch (operation) {
        case "add":
            result = valueA + valueB;
            break;
        case "subtract":
            result = valueA - valueB;
            break;
        case "multiply":
            result = valueA * valueB;
            break;
        case "divide":
            result = valueB !== 0 ? valueA / valueB : "Error: Division by zero";
            break;
        default:
            result = "Error: Unknown operation";
    }
    
    console.log(`Math: ${valueA} ${operation} ${valueB} = ${result}`);
    
    return {
        result: result,
        operation: operation,
        timestamp: new Date().toISOString()
    };
}

// Custom node type: String Processor
function stringProcessorNode(inputs) {
    console.log("String Processor Node processing");
    
    const text = inputs.text || "";
    const operation = inputs.operation || "uppercase";
    
    let result;
    
    switch (operation) {
        case "uppercase":
            result = text.toUpperCase();
            break;
        case "lowercase":
            result = text.toLowerCase();
            break;
        case "reverse":
            result = text.split("").reverse().join("");
            break;
        case "length":
            result = text.length;
            break;
        case "words":
            result = text.split(/\s+/).filter(word => word.length > 0).length;
            break;
        default:
            result = text;
    }
    
    console.log(`String: "${text}" -> "${result}"`);
    
    return {
        result: result,
        original: text,
        operation: operation
    };
}

// Custom node type: Data Filter
function dataFilterNode(inputs) {
    console.log("Data Filter Node processing");
    
    const data = inputs.data || [];
    const filterType = inputs.filterType || "all";
    const threshold = parseFloat(inputs.threshold) || 0;
    
    let filteredData;
    
    if (!Array.isArray(data)) {
        console.log("Data Filter: Input is not an array");
        return { result: [], error: "Input must be an array" };
    }
    
    switch (filterType) {
        case "greater":
            filteredData = data.filter(item => parseFloat(item) > threshold);
            break;
        case "less":
            filteredData = data.filter(item => parseFloat(item) < threshold);
            break;
        case "equal":
            filteredData = data.filter(item => parseFloat(item) === threshold);
            break;
        case "even":
            filteredData = data.filter(item => parseFloat(item) % 2 === 0);
            break;
        case "odd":
            filteredData = data.filter(item => parseFloat(item) % 2 !== 0);
            break;
        default:
            filteredData = data;
    }
    
    console.log(`Filter: ${data.length} items -> ${filteredData.length} items (${filterType})`);
    
    return {
        result: filteredData,
        originalCount: data.length,
        filteredCount: filteredData.length,
        filterType: filterType
    };
}

// Custom node type: JSON Processor
function jsonProcessorNode(inputs) {
    console.log("JSON Processor Node processing");
    
    const operation = inputs.operation || "parse";
    const data = inputs.data;
    
    let result;
    
    try {
        switch (operation) {
            case "parse":
                if (typeof data === "string") {
                    result = JSON.parse(data);
                } else {
                    result = data;
                }
                break;
            case "stringify":
                result = JSON.stringify(data, null, 2);
                break;
            case "keys":
                if (typeof data === "object" && data !== null) {
                    result = Object.keys(data);
                } else {
                    result = [];
                }
                break;
            case "values":
                if (typeof data === "object" && data !== null) {
                    result = Object.values(data);
                } else {
                    result = [];
                }
                break;
            default:
                result = data;
        }
        
        console.log(`JSON: ${operation} operation completed`);
        
        return {
            result: result,
            operation: operation,
            success: true
        };
        
    } catch (error) {
        console.log(`JSON Error: ${error.message}`);
        
        return {
            result: null,
            operation: operation,
            success: false,
            error: error.message
        };
    }
}

// Custom node type: Timer/Delay
function timerNode(inputs) {
    console.log("Timer Node processing");
    
    const delay = parseInt(inputs.delay) || 1000;
    const data = inputs.data;
    
    // Note: In a real implementation, this would use actual timing
    // For now, we'll simulate the delay behavior
    console.log(`Timer: Simulating ${delay}ms delay`);
    
    return {
        result: data,
        delay: delay,
        timestamp: new Date().toISOString(),
        message: `Data delayed by ${delay}ms`
    };
}

// Custom node type: Random Generator
function randomGeneratorNode(inputs) {
    console.log("Random Generator Node processing");
    
    const type = inputs.type || "number";
    const min = parseFloat(inputs.min) || 0;
    const max = parseFloat(inputs.max) || 100;
    const count = parseInt(inputs.count) || 1;
    
    let result;
    
    switch (type) {
        case "number":
            if (count === 1) {
                result = Math.random() * (max - min) + min;
            } else {
                result = Array.from({ length: count }, () => Math.random() * (max - min) + min);
            }
            break;
        case "integer":
            if (count === 1) {
                result = Math.floor(Math.random() * (max - min + 1)) + min;
            } else {
                result = Array.from({ length: count }, () => Math.floor(Math.random() * (max - min + 1)) + min);
            }
            break;
        case "boolean":
            if (count === 1) {
                result = Math.random() > 0.5;
            } else {
                result = Array.from({ length: count }, () => Math.random() > 0.5);
            }
            break;
        case "uuid":
            // Simple UUID-like generator
            const generateUUID = () => {
                return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
                    const r = Math.random() * 16 | 0;
                    const v = c === 'x' ? r : (r & 0x3 | 0x8);
                    return v.toString(16);
                });
            };
            
            if (count === 1) {
                result = generateUUID();
            } else {
                result = Array.from({ length: count }, generateUUID);
            }
            break;
        default:
            result = Math.random();
    }
    
    console.log(`Random: Generated ${type} value(s)`);
    
    return {
        result: result,
        type: type,
        count: count,
        range: { min, max }
    };
}

// Node type registry
const customNodeTypes = {
    "MathCalculator": mathCalculatorNode,
    "StringProcessor": stringProcessorNode,
    "DataFilter": dataFilterNode,
    "JSONProcessor": jsonProcessorNode,
    "Timer": timerNode,
    "RandomGenerator": randomGeneratorNode
};

// Function to execute a custom node
function executeCustomNode(nodeType, inputs) {
    console.log(`Executing custom node: ${nodeType}`);
    
    const nodeFunction = customNodeTypes[nodeType];
    
    if (!nodeFunction) {
        console.log(`Error: Unknown custom node type: ${nodeType}`);
        return {
            result: null,
            error: `Unknown node type: ${nodeType}`,
            success: false
        };
    }
    
    try {
        const result = nodeFunction(inputs);
        return {
            ...result,
            success: true,
            nodeType: nodeType
        };
    } catch (error) {
        console.log(`Error executing ${nodeType}: ${error.message}`);
        return {
            result: null,
            error: error.message,
            success: false,
            nodeType: nodeType
        };
    }
}

// Function to get available custom node types
function getAvailableNodeTypes() {
    return Object.keys(customNodeTypes);
}

// Export for use in the application
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        customNodeTypes,
        executeCustomNode,
        getAvailableNodeTypes
    };
}