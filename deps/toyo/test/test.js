if (process.platform === 'win32') {
  let addon
  if (process.arch === 'x64') {
    addon = require('../build/win/x64/Debug/addon.node')
  } else {
    addon = require('../build/win/Win32/Debug/addon.node')
  }

  addon.run()
}
