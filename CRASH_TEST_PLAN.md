# 💥 Crash Test Execution Plan

## 🎯 **Objective**
Systematically test failure modes to determine if the application crashes or gracefully handles errors.

## 📋 **Test Sequence**

### **Test 1: Malformed XML Structure**
```cmd
NodeGraph.exe ..\..\crash_test_1_malformed_xml.xml
```
**Target**: libxml2 parser failure
**Expected**: Either crash or graceful "file not loaded" error

### **Test 2: Duplicate Node IDs**
```cmd
NodeGraph.exe ..\..\crash_test_2_duplicate_ids.xml
```
**Target**: Hash collision in QHash<QUuid, Node*>
**Expected**: Either crash or last-wins behavior

### **Test 3: Integer Overflow Values**
```cmd
NodeGraph.exe ..\..\crash_test_3_integer_overflow.xml
```
**Target**: Coordinate/socket index bounds checking
**Expected**: Either crash or clamped values

### **Test 4: Null Injection**
```cmd
NodeGraph.exe ..\..\crash_test_4_null_injection.xml
```
**Target**: String handling with empty/null values
**Expected**: Either crash or empty node creation

### **Test 5: Circular Self-Reference**
```cmd
NodeGraph.exe ..\..\crash_test_5_circular_edges.xml
```
**Target**: Self-connecting edge logic
**Expected**: Either crash or rejected connection

### **Test 6: Empty Attribute Values**
```cmd
NodeGraph.exe ..\..\crash_test_6_empty_values.xml
```
**Target**: Attribute parsing with empty strings
**Expected**: Either crash or validation rejection

### **Test 7: Unicode Character Chaos**
```cmd
NodeGraph.exe ..\..\crash_test_7_unicode_chaos.xml
```
**Target**: Unicode string handling in UUIDs
**Expected**: Either crash or string processing error

### **Test 8: Original Foobared Test**
```cmd
NodeGraph.exe ..\..\foobared_test.xml
```
**Target**: Multiple simultaneous validation failures
**Expected**: Graceful degradation with detailed logging

## 📊 **Result Analysis Framework**

For each test, record:

### **Application Behavior**
- [ ] **CRASH**: Application terminates unexpectedly
- [ ] **GRACEFUL**: Shows error dialog/message and continues
- [ ] **SILENT**: Loads empty graph without errors
- [ ] **PARTIAL**: Loads some elements, rejects others

### **Log Evidence**
- Check latest log file for error messages
- Note if crash occurs before logging
- Document any memory access violations

### **Expected Outcomes**

| Test | Crash Risk | Graceful Handling Expected |
|------|------------|----------------------------|
| 1 - Malformed XML | LOW | ✅ libxml2 should handle |
| 2 - Duplicate IDs | MEDIUM | ⚠️ Hash collision possible |
| 3 - Integer Overflow | HIGH | ❌ No bounds checking |
| 4 - Null Injection | MEDIUM | ⚠️ QString usually safe |
| 5 - Circular Edges | LOW | ✅ Should be rejected |
| 6 - Empty Values | MEDIUM | ⚠️ Depends on validation |
| 7 - Unicode Chaos | LOW | ✅ Qt handles Unicode well |
| 8 - Foobared | LOW | ✅ Multiple validations |

## 🎯 **Success Criteria**

**Robust Application**: All tests result in graceful error handling with informative messages.

**Fragile Application**: Any test causes crashes or hangs.

**Post-Test Actions**: Document crash points and implement additional validation for any crash-causing scenarios.