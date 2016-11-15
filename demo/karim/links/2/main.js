const {app, ipcMain, BrowserWindow}  = require('electron');

const config = require('./config.json');
const appPath = process.argv[2];

let protocol;

console.log("appPath : ", appPath);

if (appPath) {
	protocol = require('url').parse(appPath).protocol;
}

let webviewPath = null;
let appConfig = null;

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
app.commandLine.appendSwitch('--touch-events');
app.commandLine.appendSwitch('--enable-transparent-visuals');
app.commandLine.appendSwitch('--ignore-certificate-errors');
app.commandLine.appendSwitch('--ppapi-flash-path', '/usr/lib/libpepflashplayer.so');

if (appPath) {
	if (protocol === 'http:' || protocol === 'https:' || protocol === 'file:') {
		webviewPath = appPath;
	} else {
		appConfig = require(appPath + '/config.json');
		let protocol = require('url').parse(appConfig.url).protocol;
		if (protocol === 'http:' || protocol === 'https:') {
			webviewPath = appConfig.url;
		} else {
			webviewPath = 'file://' + appPath + '/' + appConfig.url;
		}
	}

} 

function createWindow () {
  // Create the browser window.
  mainWindow = new BrowserWindow({
		width: appConfig && appConfig.width? appConfig.width: config.width,
		height: appConfig && appConfig.height? appConfig.height: config.height,
		transparent: true,
		frame: false,
		fullscreenable: false,
		plugins: true,
		webPreferences: {
			nodeIntegration: true
		}
	});

  // and load the index.html of the app.
  mainWindow.loadURL('file://' + __dirname + '/' + config.filename);

  // Open the DevTools.
	if (config.debug) {
  	mainWindow.webContents.openDevTools()
	}

  // Emitted when the window is closed.
  mainWindow.on('closed', function () {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null
  })

  mainWindow.webContents.on('did-finish-load', () => {
		mainWindow.webContents.send('asynchronous-message', webviewPath);
	});

	mainWindow.webContents.on('new-window', (event, urlToOpen) => {
		event.preventDefault();
		console.log('url to open: ', urlToOpen);
	});

	mainWindow.on('blur', () => {
		console.log('win blur');
	});

	mainWindow.on('focus', () => {
		console.log('win focus');
	});

	mainWindow.on('resize', () => {
		console.log('win resize');
		mainWindow.webContents.send('window-resize');
	});
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow)

// Quit when all windows are closed.
app.on('window-all-closed', function () {
  // On OS X it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', function () {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (mainWindow === null) {
    createWindow()
  }
})

app.on('browser-window-blur', function () {
	console.log('browser window blur');
});

app.on('browser-window-focus', function () {
	console.log('browser window focus');
});

ipcMain.on('asynchronous-message', (event, arg) => {
	console.log('recived msg from renderer process: ', arg);
});

ipcMain.on('webview-input-focus', (event, arg) => {
	// TODO we need to launch nemo keyboard 
	console.log('recieved from renderer: ', arg);
});
