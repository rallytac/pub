# Engage API artifacts

The C/C++, C#, and API reference documentation that were previously in the **c/**, **cs/**, and **doc/** directories here are published with each build on [Rallytac HQ](https://hq.rallytac.com/builds/).

| Artifact | Location |
|----------|----------|
| C/C++ | `https://hq.rallytac.com/<build_type>/<version>/c` |
| C# | `https://hq.rallytac.com/<build_type>/<version>/cs` |
| API reference (Doxygen) | `https://hq.rallytac.com/<build_type>/<version>/doc` |

Replace `<build_type>` and `<version>` with the release line and build version you are using (for example `builds/interim` and `1.259.9099`). C/C++ headers are under `c/include/`.

## Still in this repository

- **nodejs/** — Node.js native addon bindings
