# Wokwi VS Code Simulation

## Setup Instructions:

### 1. Install Wokwi Extension in VS Code
- Open VS Code
- Go to Extensions (⌘+Shift+X)
- Search "Wokwi Simulator"
- Install it

### 2. Open This Folder
```bash
cd "docs/cold_chain_project/current/wokwi_vscode"
code .
```

### 3. Start Simulation
- Press **F1** → type "Wokwi: Start Simulator"
- OR click the green play button when sketch.ino is open
- Simulation runs LOCALLY (no build queue!)

---

## Files in This Folder:

```
wokwi_vscode/
├── sketch.ino       # Main Arduino code (REQUIRED)
├── diagram.json     # Circuit schematic (REQUIRED)
└── libraries.txt   # Dependencies (REQUIRED)
```

**Note:** Wokwi VS Code extension compiles automatically - no build config needed!

---

## How It Works:

✅ **Compiles locally** - No online queue wait  
✅ **Runs in VS Code** - Full debugging  
✅ **Interactive circuit** - Click components  
✅ **Serial monitor** - Real-time output  

---

## Demo Steps:

1. **Start Simulation** (F1 → Wokwi: Start Simulator)
2. **Observe LCD** showing status
3. **Adjust Potentiometers** (temperature)
4. **Click MPU6050** to simulate vibration
5. **Watch Serial Monitor** for logs
6. **Screenshot** for report

---

## Troubleshooting:

**Issue:** Simulation won't start  
**Fix:** Make sure Wokwi extension is installed and enabled

**Issue:** "Compilation failed"  
**Fix:** Check libraries.txt has "LiquidCrystal I2C"

**Issue:** MPU6050 not responding  
**Fix:** Click on the MPU6050 component to interact

---

## Alternative: Use Wokwi.com

If VS Code extension has issues:
1. Go to https://wokwi.com
2. Create new ESP32 project
3. Copy sketch.ino content
4. Replace diagram.json
5. Add library in dependencies

The code is the same, just faster in VS Code locally!
