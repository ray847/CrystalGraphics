# Documents

## QuickStart

```c++
/* This includes everything from this library. */
#include <CrystalGraphics/graphics.h>

int main() {

    /* Build a scene to render. */
    crystal::graphics::Model model{"model.xxx"};
    crystal::graphics::Scene scene{};
    scene.Insert(model);

    /* Open a window to render to. */
    crystal::graphics::Window window{};
    window.View(scene);

    return 0;
}
```

## General

## Concepts

There are a few concepts this library introduces that the user should be aware
of for basic usage.

* Windows

