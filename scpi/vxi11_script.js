'use strict';

// Example Duktape hook. Return a string to handle a command, or null to fall back.
function handle(cmd) {
  var up = cmd.trim().toUpperCase();

  if (up === ':SIM:TRACE:TRI') {
    vxi.log('info', 'JS generating triangle trace');
    var points = vxi.getTracePoints();
    var trace = new Array(points);
    for (var i = 0; i < points; i++) {
      var t = points > 1 ? (i / (points - 1)) : 0;
      trace[i] = -90 + 10 * Math.sin(t * Math.PI * 2);
    }
    vxi.setTrace(trace);
    return 'OK';
  }

  if (up === ':SIM:TONE:OFF') {
    vxi.log('info', 'JS tone disabled');
    vxi.setToneEnabled(false);
    vxi.generateTrace();
    return 'OK';
  }

  if (up === ':SIM:TONE:ON') {
    vxi.log('info', 'JS tone enabled');
    vxi.setToneEnabled(true);
    vxi.generateTrace();
    return 'OK';
  }

  return null;
}
