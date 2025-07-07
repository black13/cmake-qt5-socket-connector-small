# UI SELECTION & DELETION CHECKPOINTS

## Step 2: Mouse Selection Checkpoints
- [ ] CP2.1: Mouse click on node triggers NodeItem::mousePressEvent()
- [ ] CP2.2: Scene clearSelection() called successfully  
- [ ] CP2.3: NodeItem::setSelected(true) called and visual update triggered
- [ ] CP2.4: Selection state change logged with debug output
- [ ] CP2.5: Mouse click on connection triggers ConnectionItem::mousePressEvent()
- [ ] CP2.6: ConnectionItem::setSelected(true) called and visual update triggered
- [ ] CP2.7: Selection highlighting visible (blue borders, red dashed lines)

## Step 3: Delete Key Checkpoints  
- [ ] CP3.1: Delete key press triggers GraphView::keyPressEvent()
- [ ] CP3.2: deleteSelected() method called successfully
- [ ] CP3.3: Scene iteration collects selected items safely
- [ ] CP3.4: Selected nodes found and collected for deletion
- [ ] CP3.5: Selected connections found and collected for deletion  
- [ ] CP3.6: GraphManager::deleteConnection() called for each selected connection
- [ ] CP3.7: GraphManager::deleteNode() called for each selected node
- [ ] CP3.8: Observer notifications fired for all deletions
- [ ] CP3.9: Memory counters remain balanced after deletion
- [ ] CP3.10: Debug status shows correct item counts

## Unified Operation Path Checkpoints
- [ ] CP4.1: UI operations use GraphManager::createNode/createConnection 
- [ ] CP4.2: UI operations use GraphManager::deleteNode/deleteConnection
- [ ] CP4.3: All operations trigger observer notifications
- [ ] CP4.4: Observer system logs all graph changes
- [ ] CP4.5: Memory leak detection passes for all operations
- [ ] CP4.6: Static counters match manager tracking

## Critical Safety Checkpoints
- [ ] CPS.1: No scene iteration during item modification
- [ ] CPS.2: No heap corruption or memory access violations
- [ ] CPS.3: Qt parent-child relationships maintained correctly
- [ ] CPS.4: Observer notifications complete without crashes
- [ ] CPS.5: String operations complete without QString corruption