<br>
<p align="center">
  <img width=384 src="https://download.nap-labs.tech/identity/svg/logos/nap_logo_blue.svg">
</p>
	
# Description

Adds [OpenCV](https://opencv.org/) computer vision functionality to NAP

## Installation
Compatible with NAP 0.6 and higher - [package release](https://github.com/napframework/nap/releases) and [source](https://github.com/napframework/nap) context. 

### From ZIP

[Download](https://github.com/naivisoftware/napopencv/archive/refs/heads/main.zip) the module as .zip archive and install it into the nap `modules` directory:
```
cd tools
./install_module.sh ~/Downloads/napopencv-main.zip
```

### From Repository

Clone the repository and setup the module in the nap `modules` directory.

```
cd modules
clone https://github.com/naivisoftware/napopencv.git
./../tools/setup_module.sh napopencv
```

## Demo

The demo shows a set of detected (classified) objects in 3D and 2D. 
Objects are detected using an OpenCV `HaarCascade Classifier`. 
The classified objects are rendered to texture and the viewport. This demo makes use of your first (default) Web-cam if present.

![](napopencv.webp)

