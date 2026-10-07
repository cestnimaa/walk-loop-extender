# Walk Loop Extender

Walk Loop Extender is an Unreal Engine editor plugin for turning a short root-motion walk cycle into a longer continuous walking animation asset.

It is useful when you have a short imported walk animation, for example from Mixamo, and you want the character to keep moving forward for a longer shot instead of snapping back to the start of the original loop.

## What It Does

- Duplicates the selected `AnimSequence` so the original animation stays untouched.
- Resamples the animation's bone tracks to a longer target frame count.
- Detects or uses a chosen root track and adds the original loop's travel distance on each repeated cycle.
- Keeps the walk rhythm and speed while extending the forward root motion.
- Adds editor menu actions in the Unreal toolbar and the `AnimSequence` right-click menu.
- Exposes a Blueprint-callable function for custom editor tools.

## Download

Download the ready-to-copy plugin ZIP from:

https://github.com/cestnimaa/walk-loop-extender/releases/download/v1.0.0/WalkLoopExtender-v1.0.0.zip

Or clone the repository:

```bash
git clone https://github.com/cestnimaa/walk-loop-extender.git
```

## Requirements

- Unreal Engine 5.8.1 or newer recommended.
- A C++ Unreal project, or a Blueprint project that can rebuild C++ plugins.
- A source animation saved as an `AnimSequence`.

## Installation

1. Download or clone this repository.
2. Copy the `WalkLoopExtender` plugin folder into your Unreal project:

   ```text
   YourProject/Plugins/WalkLoopExtender
   ```

3. Open the project in Unreal Engine.
4. When Unreal asks to rebuild modules, choose **Yes**.
5. Enable **Walk Loop Extender** from **Edit > Plugins** if it is not already enabled.
6. Restart the editor if Unreal asks.

## How To Use

1. Import your walk animation into Unreal.
2. In the Content Browser, select the short `AnimSequence`.
3. Run either command:
   - **Tools > Walk Loop Extender > Create 600-Frame Walk Loop**
   - Right-click the animation asset, then choose **Walk Loop Extender > Create 600-Frame Walk Loop**
4. The plugin creates a new animation in the same folder with `_Loop600` added to the name.
5. Open the new animation and preview it with root motion enabled.

## Settings

Open **Project Settings > Plugins > Walk Loop Extender**.

- **Target Frame Count**: The length of the generated animation. Default: `600`.
- **Root Bone Name**: Leave empty for auto-detect. If the character does not travel correctly, set this to the moving root track. Mixamo-style assets often use `Hips`; root-motion rigs often use `root`.
- **Flatten Accumulated Z**: Enabled by default to prevent tiny vertical offsets from building up over many repeated loops.
- **Output Suffix**: Text added to the duplicated animation name. Default: `_Loop600`.

## How It Works

The plugin reads the source animation's bone tracks, samples them across the requested output length, and repeats the local pose motion. For the selected root track, it calculates the travel distance of the original loop and adds that distance after each completed cycle. The result is a longer animation that preserves the original walk timing while continuing forward.

## Notes And Limitations

- The original animation asset is not modified.
- The generated animation is baked into a duplicated `AnimSequence`.
- Animation curves and notifies are copied by Unreal's duplicate step, but this version does not tile curve keys or notify events across every repeated loop.
- If the output does not move forward correctly, set **Root Bone Name** manually in the plugin settings.

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.
