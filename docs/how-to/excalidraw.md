# Excalidraw Diagrams

Start off by creating your diagram in <https://excalidraw.com>

```{raw} html
:file: ../images/excalidraw-example.svg
```

Click 'Save as image' and make sure the 'Embed scene' checkbox is enabled. This is
required for loading your image back into Excalidraw should you wish to make changes
later on. Name your file and export to SVG, saving it inside `docs/images`.

## Embed

Add the following to embed it inside your documentation:

``````
```{raw} html
:file: ../images/excalidraw-example.svg
```
``````

It is preferred to use this convention over `![]()` in order to retain the font used by
Excalidraw.

## VSCode Excalidraw Editor

Excalidraw images can be edited within vscode using an [extension](https://marketplace.visualstudio.com/items?itemName=pomdtr.excalidraw-editor).
The entries in `.vscode/settings.json` ensure that the extension is launched for the
`.svg` files and that the export configuration is set correctly. If `Ctrl+S` save the
image with a background, try exporting and saving through file browser instead (ensuring
to select `Embed scene` and deselect `Background`).
