# PWNG Client

pwng-client is a default client for displaying information from a galaxy-down-to-rigid-body simulation engine pwng will eventually become.

See **[pwng-server](https://github.com/planeworld/pwng-server)** for detailed documentation about Planeworld NG.

![build](https://github.com/planeworld/pwng-client/actions/workflows/ci.yml/badge.svg)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/8d8325f947844b9f86d0947d28b6692f)](https://www.codacy.com/gh/planeworld/pwng-client/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=planeworld/pwng-client&amp;utm_campaign=Badge_Grade)

Since the default desktop client shows a lot of the underlying server simulation, all information can be found there: [pwng-server](https://github.com/planeworld/pwng-server). A teaser can be seen in the following Figure 1:

![Very early galaxy representation](screenshots/galaxy.png?raw=true)

*Figure 1: Galaxy representation using Magnum.Graphics and EnTT. The galaxy is generated procedurally by incoporating distributions of stars and their features from information publicy available*

## Branches

The *master* branch uses SDL2 to initialise a window and an OpenGL context. If you prefer GLFW over SDL2, feel free to choose the *glfw* branch.
