Pebble.addEventListener("ready", function (e) {
    console.log("Connect! " + e.ready);
});

Pebble.addEventListener("showConfiguration", function () {
//    console.log("Configuration window launching...");
    Pebble.openURL("http://www.cyn.org/pebble/timely/2.1.0.html" + '?_=' + new Date().getTime());
});

function saveBatteryValue(e) {
    console.log("Battery: " + e.payload.send_batt_percent + "%, Charge: " + e.payload.send_batt_charging + ", Plugged: " + e.payload.send_batt_plugged);
/*
var currentdate = new Date(); 
var datetime = "Date: " + currentdate.getDate() + "/"
                + (currentdate.getMonth()+1)  + "/" 
                + currentdate.getFullYear() + " @ "  
                + currentdate.getHours() + ":"  
                + currentdate.getMinutes() + ":" 
                + currentdate.getSeconds();
console.log(datetime);
*/
    // TODO - actually store these in localStorage along with a date object in some useful manner
}

function sendTimezoneToWatch() {
    var offsetHours = new Date().getTimezoneOffset() / 60;
    // 5 means GMT-5, -5 means GMT+5 ... -12 through +14 are the valid options
    Pebble.sendAppMessage({ message_type: 103, timezone_offset: offsetHours },
        function (e) {
            console.log("Sent TZ message (" + offsetHours + ") with transactionId=" + e.data.transactionId);
        },
        function (e) {
            console.log("Unable to deliver TZ message with transactionId=" + e.data.transactionId + " Error is: " + e.data.error.message);
        }
        );
}

Pebble.addEventListener("appmessage", function (e) {
    //console.log("Received message: type " + e.payload.message_type)
    switch (e.payload.message_type) {
    case 100:
        saveBatteryValue(e);
        break;
    case 103:
        sendTimezoneToWatch();
        break;
    }
});

Pebble.addEventListener("webviewclosed", function (e) {
    //console.log("Configuration closed");
    //console.log(e.response);
    if (e.response !== undefined) { // user clicked Save/Submit, not Cancel/Done
        var options = JSON.parse(decodeURIComponent(e.response));
        //console.log("Options = " + JSON.stringify(options));
        Pebble.sendAppMessage(options,
            function (e) {
                console.log("Successfully delivered message with transactionId=" + e.data.transactionId);
            },
            function (e) {
                console.log("Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.data.error.message);
            }
            );
    }
});
