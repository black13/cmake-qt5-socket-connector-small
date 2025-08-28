# 📋 Manual Testing Checklist with Log Verification

## 🎯 **Testing Goal**
Systematically verify NodeGraph core functionality through manual testing with log file verification.

## 📁 **Test Environment Setup**

### **Windows Testing**
```cmd
cd C:\temp\cmake-qt5-socket-connector-small\build_Debug\Debug
# or
cd C:\temp\cmake-qt5-socket-connector-small\build_Release\Release
```

### **WSL Testing** 
```bash
cd /mnt/c/temp/cmake-qt5-socket-connector-small/build_linux
export QT_QPA_PLATFORM=offscreen  # for headless testing
```

## ✅ **Test Checklist**

### **Test 1: Basic Application Startup**
**Goal**: Verify application initializes correctly

**Steps**:
1. Run: `./NodeGraph --version`  
2. Run: `./NodeGraph --help`
3. Run: `./NodeGraph` (let it start, then close immediately)

**Expected Log Evidence**:
- Application version logged
- Help text displayed  
- Normal startup sequence without crashes
- Clean shutdown logged

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 2: XML File Loading - Simple Test**
**Goal**: Verify basic XML loading functionality

**Steps**:
1. Run: `./NodeGraph ../simple_test.xml`
2. Let application load (5-10 seconds)
3. Close application

**Expected Log Evidence**:
- XML file parsing logged
- Node count: 2 nodes loaded
- Edge count: 1 edge loaded  
- No XML parsing errors

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 3: XML File Loading - Complex Test**
**Goal**: Test multi-node graph loading

**Steps**:
1. Run: `./NodeGraph ../complex_test.xml`  
2. Let application load completely
3. Close application

**Expected Log Evidence**:
- XML parsing success logged
- Node count: 5 nodes loaded
- Edge count: 5 edges loaded
- All node types recognized (SOURCE, SINK, etc.)

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 4: Performance Test - Tiny Graph**
**Goal**: Test small automated graph performance

**Steps**:
1. Run: `./NodeGraph ../tests_tiny.xml`
2. Let application load (should be quick)
3. Close application

**Expected Log Evidence**:
- 10 nodes loaded successfully  
- 9 edges loaded successfully
- Loading time under 1 second
- No memory warnings

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 5: Performance Test - Small Graph** 
**Goal**: Test moderate load performance

**Steps**:
1. Run: `./NodeGraph ../tests_small.xml`
2. Wait for complete loading
3. Close application  

**Expected Log Evidence**:
- 100 nodes loaded successfully
- 99 edges loaded successfully  
- Loading time reasonable (under 5 seconds)
- Memory usage acceptable

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 6: Performance Test - Medium Graph**
**Goal**: Test significant load handling

**Steps**:
1. Run: `./NodeGraph ../tests_medium.xml`
2. Wait for loading to complete  
3. Close application

**Expected Log Evidence**:
- 500 nodes loaded successfully
- 499 edges loaded successfully
- Application remains responsive
- No crashes or memory issues

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 7: Stress Test - Large Graph**
**Goal**: Test high-load performance  

**Steps**:
1. Run: `./NodeGraph ../tests_large.xml`
2. Wait for loading (may take time)
3. Close when loaded

**Expected Log Evidence**:
- 1000 nodes loaded successfully
- 999 edges loaded successfully  
- Application handles load without crashing
- Memory management working

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 8: Interactive GUI Test** (Windows Only)
**Goal**: Test user interface functionality

**Steps** (Windows GUI only):
1. Run: `NodeGraph.exe` (with GUI)
2. Try creating a node from palette
3. Try connecting two nodes
4. Save as `manual_test.xml`
5. Close application

**Expected Log Evidence**:
- GUI initialization logged
- Node creation events logged
- Edge creation events logged  
- XML save operation logged
- Clean GUI shutdown

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 9: File Save/Load Roundtrip**
**Goal**: Verify XML persistence works both ways

**Steps**:
1. Run: `./NodeGraph ../simple_test.xml`
2. If GUI available: make small change, save as `roundtrip_test.xml`
3. Close application
4. Run: `./NodeGraph ../roundtrip_test.xml` (or use original if no GUI)
5. Close application

**Expected Log Evidence**:
- Original file loaded correctly
- Save operation completed (if GUI available)
- Reload shows same node/edge counts
- Data integrity maintained

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

### **Test 10: Error Handling Test**
**Goal**: Test graceful error handling

**Steps**:
1. Run: `./NodeGraph nonexistent_file.xml`
2. Run: `./NodeGraph ../README.md` (invalid XML)  
3. Run: `./NodeGraph --invalid-flag`

**Expected Log Evidence**:
- File not found error handled gracefully
- Invalid XML error reported clearly
- Invalid arguments handled properly
- No crashes, proper error messages

**Status**: [ ] COMPLETED - **Timestamp**: ___________

---

## 🔍 **Log Verification Commands**

After running tests, I'll check logs with:

```bash
# Check latest log file
ls -la logs/ | tail -5

# Search for specific events  
grep -i "nodes loaded" logs/NodeGraph_*.log
grep -i "edges loaded" logs/NodeGraph_*.log
grep -i "error\|warning" logs/NodeGraph_*.log
grep -i "xml" logs/NodeGraph_*.log
```

## 📊 **Completion Summary**

**Total Tests**: 10  
**Completed**: ___/10  
**Failed**: ___/10  
**Platform Tested**: [ ] Windows [ ] WSL [ ] Both

**Overall Result**: [ ] PASS [ ] FAIL  

**Notes**:
_________________________________________________
_________________________________________________
_________________________________________________

## 🎯 **Next Steps After Testing**
1. Review log files for any issues
2. Address any failed tests  
3. Plan GUI-specific testing if needed
4. Validate architecture under load