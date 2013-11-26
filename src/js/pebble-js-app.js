Pebble.addEventListener("ready", function(e) {
//  console.log("Connect! " + e.ready);
});

Pebble.addEventListener("showConfiguration", function(e) {
    console.log("Configuration window launching...");
    Pebble.openURL("http://www.cyn.org/pebble/timely/2.0.0.html");
});

Pebble.addEventListener("webviewclosed", function(e) {
//  console.log("Configuration closed");
  var options = JSON.parse(decodeURIComponent(e.response));
//  console.log("Options = " + JSON.stringify(options));
  Pebble.sendAppMessage( options );
/*
  var transactionId = Pebble.sendAppMessage( JSON.stringify(options),
  function(e) {
    console.log("Successfully delivered message with transactionId="
      + e.data.transactionId);
  },
  function(e) {
    console.log("Unable to deliver message with transactionId="
      + e.data.transactionId
      + " Error is: " + e.error.message);
  }
);
//*/
});
