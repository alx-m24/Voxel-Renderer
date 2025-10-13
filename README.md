# Voxel Renderer

A lightweight **.lib** library for rendering voxel data.

---

## - Overview

This library is built on top of **LexviEngine** and serves as a minimal port of my earlier **Voxel Engine**, focused purely on voxel rendering.

It’s designed to integrate seamlessly with LexviEngine projects—just include the Voxel Renderer class and call its member functions as needed.

---

## - Features

- Reflections  
- Transparency  
- Shadows  
- Other basic visual effects  

> ⚠️ Note: This renderer **does not** include physics or particle systems.  
> It’s a dedicated **rendering module**, representing only one subsystem from the full Voxel Engine.

---

## - Usage

1. **Initialize the Renderer**
   - You can initialize with:
     - Existing voxel data (as **RGB + Reflection** or **Alpha**)
     - Empty voxel grid (to be filled later)

2. **Specify Dimensions**
   - Define the voxel grid size and resolution.

3. **Update Data**
   - Modify or refresh the voxel grid as needed.

4. **Draw**
   - Issue draw calls to render the scene.

---

## - Integration Notes

- Designed as an **extension of LexviEngine**.  
- Use it the same way you’d use other LexviEngine modules:
  - Include the Voxel Renderer header/class.
  - Call its member functions within your existing engine loop or scene system.

---

### - Summary

This project offers a **focused, efficient voxel rendering solution**—ideal when you want the visual core of a voxel engine without the overhead of physics or simulation systems.
