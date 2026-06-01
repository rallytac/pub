# Engage API artifacts

The C/C++ and C# API sources that were previously in the **c/** and **cs/** directories here are published with each build on [Rallytac HQ](https://hq.rallytac.com/builds/).

| API | Location |
|-----|----------|
| C/C++ | `https://hq.rallytac.com/<build_type>/<version>/c` |
| C# | `https://hq.rallytac.com/<build_type>/<version>/cs` |

Replace `<build_type>` and `<version>` with the release line and build version you are using (for example `builds/interim` and `1.259.9099`). C/C++ headers are under `c/include/`.

## Still in this repository

- **doc/** — generated API reference (Doxygen)
- **nodejs/** — Node.js native addon bindings
