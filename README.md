# Template for CS2 Metamod plugins with windows & linux build support.
Thank you to GAMMACASE for updating the s2 sample plugin and CS2Fixes for the relevant docker files :)

This project solution is intended to be used with visual studio code. If you want to generate a visual studio solution instead, you can delete the `.vscode` directory and follow the instructions in GAMMACASE's updated s2 sample plugin located on the newest metamod-source branch.

## Copying the template
Ensure that you clone with the `--recursive` option. i.e.: `git clone --recursive https://github.com/P250/MetamodTest`. This will download the submodules as well, which are required for compiling.

## Compiling for Linux
Simply run `docker compose up` in the project directory. Note that you must have the hl2sdk (cs2 branch) in your project, specifically named `hl2sdk-cs2`, as well as your `hl2sdk-manfiests` named folder in the project directory. I use git submodules for this.
