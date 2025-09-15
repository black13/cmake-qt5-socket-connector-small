#pragma once

class Scene;
class GraphFactory;

/**
 * Header-only stub for GraphScriptApi
 * 
 * This stub exists so Window members remain stable whether JavaScript is ON or OFF.
 * When ENABLE_JS is OFF, this provides placeholder members that compile to nothing.
 * When ENABLE_JS is ON, the real implementation in graph_script_api.h/.cpp is used.
 */
class GraphScriptApiStub {
public:
    explicit GraphScriptApiStub(Scene*, GraphFactory*) {
        // Empty constructor - no JavaScript functionality
    }
    
    // No methods when JS is disabled
    // Window can safely hold a pointer to this stub
};