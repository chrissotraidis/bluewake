# Historical RecompCore patches

These patches record BlueWake's RecompCore changes as they were made. They are history, not a build
input: the series starts at 0008 (0001-0007 were never exported), so it does not apply to the
upstream base 5c3611e, and the local head it led to (3476998) was never published.

The build uses a fork instead. BlueWake's is https://github.com/chrissotraidis/RecompCore, branch
`bluewake`, commit 2d6063614a9bc899f6b4d11c7e7b3cd66e4d96f3: it contains the changes here through 0097
(some were revised by later ones), the files that were never committed on the development Mac, and the
DolRecomp submodule pointing at https://github.com/chrissotraidis/DolRecomp (5c91d6e). Wind Waker Recomp
builds from its own copy, https://github.com/elliotttate/RecompCore, branch `bluewake`, commit
8ab24da: that tree plus 0098 to 0112, with DolRecomp at https://github.com/elliotttate/DolRecomp
(b8b5345, 5c91d6e plus patches/dolrecomp/0019). Its `windows-release` branch adds 0113 (9618e9d,
the render worker paused while the swapchain changes). BlueWake now builds from
https://github.com/chrissotraidis/RecompCore, branch `bluewake-next`: 9618e9d plus 0114 (one DSP
interpreter table layout under the Microsoft ABI, so Exact/LLE audio no longer calls address 0
on Windows), with the same DolRecomp. The Builder fetches it at the commit pinned in
`scripts/builder/profiles/bluewake.sh`; see docs/status/DEVICE_BUILD.md.
