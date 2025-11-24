# ![HdriVault Preview](Resources/Icon128.png) 
# HdriVault

**HdriVault** is an open-source plugin for Unreal Engine 5 that acts as a centralized manager for your HDRI library. It simplifies the process of importing, organizing, visualizing, and applying panoramic HDRI textures to your scenes.


## Features

### 📂 Centralized Asset Management
*   **Auto-Discovery**: Automatically scans your project (Content, Engine, Plugins) for all `TextureCube` assets.
*   **Dual Views**: Navigate using your familiar folder structure or organize efficiently with custom **Categories**.
*   **Tagging System**: Add custom tags to assets for quick filtering and retrieval.
*   **Metadata**: View dimensions, resource size, and add custom notes/author info that persists separately from the asset file.

### 🚀 Smart Import Workflow
*   **Drag-and-Drop**: Drag `.exr` or `.hdr` files directly from Windows Explorer into the plugin window.
*   **Auto-Conversion**: Built-in C++ converter (using `tinyexr`) automatically converts **EXR to HDR** on import—no external tools required!
*   **Batch Tagging**: Set the destination folder, category, tags, and author for a batch of files *before* import.
*   **Native Integration**: Uses Unreal's native asset pipeline but suppresses default dialogs for speed.

### 🎨 Visualization
*   **Panoramic Thumbnails**: Assets are displayed with a **2:1 aspect ratio** to properly visualize the full horizon, rather than a distorted sphere.
*   **Customizable Grid**: Adjustable thumbnail sizes and view modes.

### ⚡ Instant Lighting
*   **1-Click Apply**: Double-click any HDRI to instantly apply it to:
    *   Active `SkyLight` actors.
    *   `HDRIBackdrop` Blueprint actors.
*   **Viewport Drag-and-Drop**: Drag an HDRI from the Vault directly onto a SkyLight in your scene.
*   **Undo/Redo**: All application actions are fully transactional.

## Installation

1.  Close Unreal Engine.
2.  Download the latest release or clone this repository into your project's `Plugins` folder:
    ```bash
    YourProject/Plugins/HdriVault
    ```
3.  Regenerate project files (right-click `.uproject` -> *Generate Visual Studio project files*).
4.  Build your project solution.
5.  Enable the plugin in **Edit > Plugins > HdriVault**.

## Usage

1.  Open the tool via **Tools > Hdri Vault** or the toolbar icon.
2.  **Import**: Drag `.exr` files into the grid area. Fill out the metadata popup and click Import.
3.  **Organize**: Right-click assets to add tags or move them to categories.
4.  **Apply**: Double-click a thumbnail to update your scene's lighting.

## Compatibility

*   **Unreal Engine**: 5.0+ (Tested on 5.3, 5.4, 5.6)
*   **Platform**: Windows (Mac/Linux support planned)

## Credits

*   **TinyEXR**: Used for EXR parsing.
*   **stb_image**: Used for HDR writing.
*   Created by **Pyre Labs**.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

