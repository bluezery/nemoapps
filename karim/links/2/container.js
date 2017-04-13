const {app, ipcRenderer} = require('electron');

ipcRenderer.on('asynchronous-message', (event, arg) => {
	if (!arg) {
		ipcRenderer.send('asynchronous-message', 'ron-url');
	} else {
		webview.setAttribute('src', arg);
		ipcRenderer.send('asynchronous-message', 'success');
	}
});

function initialize() 
{
	console.log('init');
	webview = document.querySelector('webview');
	webview.addEventListener("transitionend", (event) => {
		console.log("webview transitioned!!");
		blockers = document.querySelectorAll('.blocker');
		blockers.forEach(function (blocker) {
			blocker.style.opacity = "1.0";
		});

		//quit = document.querySelector('.quit-button');
		//quit.style.opacity = "1.0";
		event.stopPropagation();
	});

	webview.style.opacity = "1.0";
	webview.style.transition = "opacity 2s";

	container = document.querySelector('.container-edge');
	container.addEventListener("transitionend", (event) => {
		console.log("container transitioned!!");
		window.close();
	});
}

function appQuit()
{
	/*
	container.style.width = "50%";
	container.style.height = "50%";
	container.style.transitionOrigin = "center center";
	container.style.transitionProperty = "2s";
	container.style.transitionTimingFunction = "ease-in-out";
	container.style.transitionDelay = "0.5s";
	*/
	container.style.opacity = "0.0";
	container.style.transition = "opacity 1s";
}
