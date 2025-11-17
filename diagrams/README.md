# Diagram Export Guide

## PlantUML Files Created

### Core Diagrams
- `state_machine.puml` - State machine diagram (3 states: IDLE, IGNITING, HEATING)
- `control_flow.puml` - Control flow diagram (startup to steady state)
- `architecture.puml` - System architecture (layers and components)

### Additional Diagrams
- `component_diagram.puml` - Component architecture with interfaces
- `fault_timing_sequence.puml` - Sensor fault detection timing sequence
- `safety_layers.puml` - Multi-layer safety validation flowchart
- `deployment.puml` - Hardware deployment architecture (Arduino MCU + peripherals)

## How to Export to Lucidchart

### Method 1: PlantUML → PNG/SVG → Lucidchart

1. **Install PlantUML:**
   ```bash
   npm install -g node-plantuml
   ```

2. **Generate PNG files:**
   ```bash
   cd diagrams
   plantuml *.puml
   ```
   This will generate PNG files for all 7 diagrams.

3. **Import to Lucidchart:**
   - Open Lucidchart
   - Click "Import"
   - Upload PNG files
   - Redraw or use as reference

### Method 2: Online PlantUML Editor

1. Go to http://www.plantuml.com/plantuml/uml/
2. Copy-paste `.puml` file content
3. Click "Submit" to render
4. Right-click image → Save
5. Import to Lucidchart

### Method 3: VS Code Extension

1. Install "PlantUML" extension in VS Code
2. Open `.puml` file
3. Press `Alt+D` to preview
4. Right-click preview → Export as PNG/SVG
5. Import to Lucidchart

### Method 4: Mermaid Live Editor (Easiest)

1. Go to https://mermaid.live
2. Copy Mermaid code from `system_design.md`
3. Click "Actions" → "Export SVG/PNG"
4. Import to Lucidchart

## Direct Lucidchart Import (Limited)

Lucidchart supports importing:
- ✅ Visio (.vsdx)
- ✅ Draw.io (.drawio)
- ❌ PlantUML (need conversion)
- ❌ Mermaid (need conversion)

## Recommended Workflow

**Best option:** Use Mermaid Live Editor
- No installation needed
- High quality SVG output
- Easy to update
- Direct copy-paste from system_design.md

**Steps:**
1. Open https://mermaid.live
2. Paste this code:
