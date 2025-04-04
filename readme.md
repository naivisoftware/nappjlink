<br>
<p align="center">
  <img width=384 src="https://download.nap-labs.tech/identity/svg/logos/nap_logo_blue.svg">
</p>
	
# Description

Allows you to remote-control projectors that support [PJLink](https://pjlink.jbmia.or.jp/english) in NAP applications.

## Installation

Compatible with NAP 0.7 and higher - [package release](https://github.com/napframework/nap/releases) and [source](https://github.com/napframework/nap) context. 

### From ZIP

[Download](https://github.com/naivisoftware/nappjlink/archive/refs/heads/main.zip) the module as .zip archive and install it into the nap `modules` directory:
```
cd tools
./install_module.sh ~/Downloads/nappjlink-main.zip
```

### From Repository

Clone the repository and setup the module in the nap `modules` directory.

```
cd modules
clone https://github.com/naivisoftware/nappjlink.git
./../tools/setup_module.sh nappjlink
```

## Usage

### Send

In Napkin:

1. add a `nap::PJLinkProjectorPool` resource
2. add a `nap::PJLinkProjector` resource
3. assign the pool (1) to the projector (2)

In your application:

```
projector = mResourceManager->findObject<PJLinkProjector>("Projector");
projector->powerOn()
```

### Receive

In Napkin:

1. add a `nap::PJLinkComponent` to an entity

In Code:

Listen to the `nap::PJLinkComponent::messageReceived` signal to receive responses:

```
#include <pjlinkcomponent.h>
...

auto& comp = mProjectorEntity->getComponent<PJLinkComponentInstance>();
comp.messageReceived.connect([](const PJLinkCommand& msg)
	{
		nap::Logger::info(msg.mResponse);
	});

```

## Structure

The `PJLinkProjector` establishes a connection (a-sync) when a message is sent, or on startup when `ConnectOnStartup` is set to true.
 
The connection remains available for 20 seconds after receiving the last response from the projector.
Subsequent messages will establish a new connection, as outlined in the pjlink protocol document.
You as a user don't have to worry about the state of the connection, that is done for you.
 
All communication is a-synchronous: all calls to `PJLinkProjector::send()` will return immediately -> the command is queued for write.
On success, the response message from the projector is forwarded to the `PJLinkComponent` that listens to this projector.
If no component is listening the response is simply discarded.

You must assign a `nap::PJLinkProjectorPool` to every projector.
The pool runs all queued I/O network requests a-synchronous on it's assigned worker thread.
1 pool per application is enough, unless you are controlling a very large (100+) number of projectors.

