from lldb import SBTarget

# prints targeted node id/type each time we hit showContextMenu
def print_context_menu(debugger, command, exe_ctx, result, internal_dict):
    frame = exe_ctx.GetFrame()
    node_val = frame.FindVariable("node")
    if node_val and node_val.GetValueAsUnsigned() != 0:
        node_id = frame.EvaluateExpression("node->getId().toString().toUtf8().constData()")
        node_type = frame.EvaluateExpression("node->getNodeType().toUtf8().constData()")
        result.PutCString(f"[LLDB] Context menu for node id={node_id.GetSummary()} type={node_type.GetSummary()}")
    else:
        result.PutCString("[LLDB] Context menu invoked (background / no node)")
    return 0
