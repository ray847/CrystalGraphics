# Source

`KhronosGroup/glTF-Sample-Assets`

### 1. The "Hello World" Tests
* **`Box` or `Triangle`:** Start here. It just proves your `cgltf` file opening, memory allocation, and basic vertex extraction loops don't segfault.
* **`BoxTextured`:** Proves you are extracting UV coordinates correctly (when you add them to your `Vertex` struct later).

### 2. The Hierarchy and Math Tests (Crucial for your Engine)
* **`CesiumMilkTruck`:** This is the gold standard for testing scene graphs. The truck body is a parent node, and the wheels are child nodes with their own local `TRS` transformations. If your `CompleteTrans` and `WorldMatrix()` recursive traversal is working, the wheels will be perfectly attached to the axles. If the math is wrong, the wheels will spawn at the origin `(0,0,0)`.
* **`Buggy`:** Similar to the milk truck, but with a much deeper and more complex node hierarchy.
* **`Cameras`:** This scene contains multiple nodes with strange rotations and scales. It is excellent for verifying that your quaternion-to-matrix conversions aren't introducing skew or shear.

### 3. The Stress Tests (For your BVH)
Once your local transformations are proven correct, you will want larger scenes to test the performance of your `TlasBuilder` and ray intersection speed.
* **`Sponza`:** This is the classic, industry-standard path tracing test scene. It has beautiful lighting opportunities, lots of overlapping bounding boxes, and around 260,000 triangles. 
* **`FlightHelmet`:** Great for testing complex, multi-primitive meshes and advanced PBR materials later on.