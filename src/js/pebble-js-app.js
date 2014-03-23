var locationOptions = { "timeout": 15000, "maximumAge": 60000, "enableHighAccuracy": false }; // 15 second timeout, allow 1 min cached
//var cachedLocationOptions = { "timeout": 0, "maximumAge": 600000, "enableHighAccuracy": false }; // allow 10 min cached
var locationWatcher;
var lastCoordinates;
var weatherFormat;

Pebble.addEventListener("ready", function (e) {
    console.log("Connect! " + e.ready);
//    locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
//    navigator.geolocation.clearWatch(locationWatcher);
});

Pebble.addEventListener("showConfiguration", function () {
    console.log("Configuration window launching...");
    var baseURL, pebtok, nocache;
    baseURL = 'http://www.cyn.org/pebble/timely/';
    pebtok  = '&pat=' + Pebble.getAccountToken();
    nocache = '&_=' + new Date().getTime();
    if (window.localStorage.getItem("timely_options") !== null) {
        options = JSON.parse(window.localStorage.timely_options);
    }
    if (window.localStorage.getItem("version_config") !== null) {
        Pebble.openURL(baseURL + window.localStorage.version_config + ".php?" + pebtok + nocache);
    } else { // in case we never received the message / new install
        Pebble.openURL(baseURL + "2.2.4.php?" + pebtok + nocache);
    }
});

function saveWatchVersion(e) {
    console.log("Watch Version: " + e.payload.send_watch_version);
    console.log("Config Version: " + e.payload.send_config_version);
    window.localStorage.version_watch = e.payload.send_watch_version;
    window.localStorage.version_config = e.payload.send_config_version;
}

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
    // TO BE DONE - actually store these in localStorage along with a date object in some useful manner
}

function sendTimezoneToWatch() {
    var offsetQuarterHours = Math.floor(new Date().getTimezoneOffset() / 15);
    // UTC offset in quarter hours ... 48 (UTC-12) through -56 (UTC+14) are the valid ranges
    Pebble.sendAppMessage({ message_type: 103, timezone_offset: offsetQuarterHours },
        function (e) {
            console.log("Sent TZ message (" + offsetQuarterHours + ") with transactionId=" + e.data.transactionId);
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
    case 104:
        saveWatchVersion(e);
        break;
    case 106:
        weatherFormat = e.payload.weather_fmt;
        console.log('Weather Format: ' + weatherFormat);
        window.navigator.geolocation.getCurrentPosition(weatherLocationSuccess, locationError, locationOptions); // request weather;
        break;
    }
});

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
//http://api.openweathermap.org/data/2.5/weather?lat=35.8415051596573&lon=-78.55771335780486&cnt=1&units=metric
//http://api.openweathermap.org/data/2.5/weather?lat=35.8415051596573&lon=-78.55771335780486&cnt=1&units=imperial
  var units = "imperial";
  if (weatherFormat === 1) { units = "metric"; }
  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1" + "&units=" + units, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        //console.log('Weather Response: ' + req.responseText);
        response = JSON.parse(req.responseText);
        var temp, temp_min, temp_max, icon, city;
        if (response && response.weather && response.weather.length > 0) {
          var weatherResult = response;
          temp = Math.round(weatherResult.main.temp);
          temp_min = Math.round(weatherResult.main.temp_min);
          temp_max = Math.round(weatherResult.main.temp_max);
          cond_main = weatherResult.weather[0].main;
          cond_desc = weatherResult.weather[0].description;
          cond_icon = iconFromWeatherId(weatherResult.weather[0].id, latitude, longitude);
          city = weatherResult.name;
/*
          console.log('temp: ' + temp);
          console.log('temp_min: ' + temp_min);
          console.log('temp_max: ' + temp_max);
          console.log('city:  ' + city);
          console.log('cond: ' + cond_main);
          console.log('cond_desc: ' + cond_desc);
          console.log('cond_icon: ' + cond_icon);
            weather_temp_min: temp_min,
            weather_temp_max: temp_max,
*/
          Pebble.sendAppMessage({
            message_type: 106,
            weather_temp: temp,
            weather_cond: cond_icon,
            });
        } else {
          for(var i in dataObj) {
                console.log('dO:' + i + ' --- ' + dataObj[i]);
            }
        }

      } else {
        console.log("Error");
      }
    }
  }
  req.send(null);
}

function weatherLocationSuccess(pos) {
  lastCoordinates = pos.coords;
  console.log('Weather: location found (' + lastCoordinates.latitude + ', ' + lastCoordinates.longitude + '): ');
  fetchWeather(lastCoordinates.latitude, lastCoordinates.longitude);
}

function locationError(err) {
  console.warn('Weather: location error (' + err.code + '): ' + err.message);
/*
  // TODO
  Pebble.sendAppMessage({
    "weath_city":"Unavailable",
    "weath_temp":"N/A"
  });
*/
}

function iconFromWeatherId(weatherId, latitude, longitude) {
  console.log("getting icon from weather");
  var now = new Date();
  var sunInfo = SunCalc.getTimes(now, latitude, longitude);
  var night = sunInfo.sunset < now || now < sunInfo.sunrise;
  var moon = "N";
  if (night) {
    var moonInfo = SunCalc.getMoonIllumination(now);
    console.log("moon: " + moonInfo.fraction + "  " + moonInfo.angle);
    if (moonInfo.fraction <= 0.05) { moon = "O"; }
    else if (moonInfo.fraction >= 0.95) { moon = "S"; }
    else if (moonInfo.angle < 0) { // waxing
      if (moonInfo.fraction <= 0.35) { moon = "P"; }
      else if (moonInfo.fraction <= 0.65) { moon = "Q"; }
      else { moon = "R"; }
    } else { // waning
      if (moonInfo.fraction <= 0.35) { moon = "T"; }
      else if (moonInfo.fraction <= 0.65) { moon = "U"; }
      else { moon = "V"; }
    }
/*
[INFO    ] JS: Timely 2.2d: moon: 0.5988898806207668  1.623056404454204
new moon = 0.0, full moon = 1.0
angle - == waxing
angle + == waning
N = generic moon/night
O = new moon        0.0
P = waxing crescent 0.25 -
Q = waxing quarter  0.5  -
R = waxing gibbous  0.75 -
S = full moon       1.00
T = waning gibbous  0.75 +
	U = waning quarter  0.50 +
V = waning crescent 0.25 +
*/
  } else {
    //console.log("daytime");
  }

  if (weatherId >= 200 && weatherId <= 232) {
    return 'F'; // thunderstorm (lightning)
  } else if (weatherId >= 300 && weatherId <= 321) {
    if (weatherId >= 313 && weatherId <= 321) {
      return "'"; // showers
    }
    return '-'; // drizzle
  } else if (weatherId >= 500 && weatherId <= 531) {
    if (weatherId >= 502 && weatherId <= 511) {
      return '*'; // heavy rain
    }
    if (weatherId >= 520 && weatherId <= 531) {
      return "'"; // showers
    }
    return '$'; // rain
  } else if (weatherId >= 600 && weatherId <= 622) {
    if (weatherId >= 611 && weatherId <= 612) {
      return '0'; // sleet
    }
    return '9'; // snow
  } else if (weatherId == 701) {
    return '?'; // !! mist
  } else if (weatherId == 711) {
    return '?'; // !! smoke
  } else if (weatherId == 721) {
    return '?'; // haze
  } else if (weatherId == 731) {
    return '?'; // !! sand/dust whirls
  } else if (weatherId == 741) {
    return '<'; // fog
  } else if (weatherId == 751) {
    return '?'; // !! sand
  } else if (weatherId == 761) {
    return '?'; // !! dust
  } else if (weatherId == 762) {
    return '?'; // !! volcanic ash
  } else if (weatherId == 771) {
    return 'B'; // squalls
  } else if (weatherId == 781) {
    return 'X'; // tornado
  } else if (weatherId == 800) {
    if (night) { return moon; } else { return 'I'; }
  } else if (weatherId >= 801 && weatherId <= 804) {
    return '!'; // clouds (varying degrees)
  } else if (weatherId == 900) {
    return 'X'; // tornado
  } else if (weatherId == 901) {
    return 'C'; // tropical storm
  } else if (weatherId == 902) {
    return 'X'; // hurricane
  } else if (weatherId == 903) {
    return 'Z'; // cold (or Y)
  } else if (weatherId == 904) {
    return ']'; // hot (or ^)
  } else if (weatherId == 905) {
    return 'B'; // windy 
  } else if (weatherId == 906) {
    return '3'; // hail
  }
  return 'h';
}

function b64_to_utf8( str ) {
  return decodeURIComponent(escape(base64.decode( str.replace(/ +/g, '+') )));
}

Pebble.addEventListener("webviewclosed", function (e) {
    //console.log("Configuration closed");
    console.log("Response = " + e.response.length + "   " + e.response);
    if (e.response !== undefined && e.response !== '' && e.response !== 'CANCELLED') { // user clicked Save/Submit, not Cancel/Done
        var options, web;
        options = JSON.parse(b64_to_utf8(e.response));
        window.localStorage.timely_options = JSON.stringify(options);
        web = options.web;
        delete options.web; // remove the 'web' object from our response, which has preferences such as language...
        options[15] = web.lang; // re-inject the language
//        if (options[10] === 1) { // debugging is on...
          console.log("Options = " + JSON.stringify(options));
//        }
        Pebble.sendAppMessage(options,
            function (e) {
                console.log("Successfully delivered message with transactionId=" + e.data.transactionId);
            },
            function (e) {
                console.log("Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.data.error.message);
            }
            );
    } else if (e.response === 'CANCELLED') {
        console.log("Android misbehaving on save due to embedded space in e.response... ignoring");
    }
});


/*
(c) 2011-2014, Vladimir Agafonkin
SunCalc is a JavaScript library for calculating sun/mooon position and light phases.
https://github.com/mourner/suncalc
*/

(function () { "use strict";

// shortcuts for easier to read formulas

var PI = Math.PI,
    sin = Math.sin,
    cos = Math.cos,
    tan = Math.tan,
    asin = Math.asin,
    atan = Math.atan2,
    acos = Math.acos,
    rad = PI / 180;

// sun calculations are based on http://aa.quae.nl/en/reken/zonpositie.html formulas


// date/time constants and conversions

var dayMs = 1000 * 60 * 60 * 24,
    J1970 = 2440588,
    J2000 = 2451545;

function toJulian(date) {
    return date.valueOf() / dayMs - 0.5 + J1970;
}
function fromJulian(j) {
    return new Date((j + 0.5 - J1970) * dayMs);
}
function toDays(date) {
    return toJulian(date) - J2000;
}


// general calculations for position

var e = rad * 23.4397; // obliquity of the Earth

function getRightAscension(l, b) {
    return atan(sin(l) * cos(e) - tan(b) * sin(e), cos(l));
}
function getDeclination(l, b) {
    return asin(sin(b) * cos(e) + cos(b) * sin(e) * sin(l));
}
function getAzimuth(H, phi, dec) {
    return atan(sin(H), cos(H) * sin(phi) - tan(dec) * cos(phi));
}
function getAltitude(H, phi, dec) {
    return asin(sin(phi) * sin(dec) + cos(phi) * cos(dec) * cos(H));
}
function getSiderealTime(d, lw) {
    return rad * (280.16 + 360.9856235 * d) - lw;
}


// general sun calculations

function getSolarMeanAnomaly(d) {
    return rad * (357.5291 + 0.98560028 * d);
}
function getEquationOfCenter(M) {
    return rad * (1.9148 * sin(M) + 0.02 * sin(2 * M) + 0.0003 * sin(3 * M));
}
function getEclipticLongitude(M, C) {
    var P = rad * 102.9372; // perihelion of the Earth
    return M + C + P + PI;
}
function getSunCoords(d) {

    var M = getSolarMeanAnomaly(d),
        C = getEquationOfCenter(M),
        L = getEclipticLongitude(M, C);

    return {
        dec: getDeclination(L, 0),
        ra: getRightAscension(L, 0)
    };
}


var SunCalc = {};


// calculates sun position for a given date and latitude/longitude

SunCalc.getPosition = function (date, lat, lng) {

    var lw = rad * -lng,
        phi = rad * lat,
        d = toDays(date),

        c = getSunCoords(d),
        H = getSiderealTime(d, lw) - c.ra;

    return {
        azimuth: getAzimuth(H, phi, c.dec),
        altitude: getAltitude(H, phi, c.dec)
    };
};


// sun times configuration (angle, morning name, evening name)

var times = [
    [-0.83, 'sunrise', 'sunset' ],
    [ -0.3, 'sunriseEnd', 'sunsetStart' ],
    [ -6, 'dawn', 'dusk' ],
    [ -12, 'nauticalDawn', 'nauticalDusk'],
    [ -18, 'nightEnd', 'night' ],
    [ 6, 'goldenHourEnd', 'goldenHour' ]
];

// adds a custom time to the times config

SunCalc.addTime = function (angle, riseName, setName) {
    times.push([angle, riseName, setName]);
};


// calculations for sun times

var J0 = 0.0009;

function getJulianCycle(d, lw) {
    return Math.round(d - J0 - lw / (2 * PI));
}
function getApproxTransit(Ht, lw, n) {
    return J0 + (Ht + lw) / (2 * PI) + n;
}
function getSolarTransitJ(ds, M, L) {
    return J2000 + ds + 0.0053 * sin(M) - 0.0069 * sin(2 * L);
}
function getHourAngle(h, phi, d) {
    return acos((sin(h) - sin(phi) * sin(d)) / (cos(phi) * cos(d)));
}


// calculates sun times for a given date and latitude/longitude

SunCalc.getTimes = function (date, lat, lng) {

    var lw = rad * -lng,
        phi = rad * lat,
        d = toDays(date),

        n = getJulianCycle(d, lw),
        ds = getApproxTransit(0, lw, n),

        M = getSolarMeanAnomaly(ds),
        C = getEquationOfCenter(M),
        L = getEclipticLongitude(M, C),

        dec = getDeclination(L, 0),

        Jnoon = getSolarTransitJ(ds, M, L);


    // returns set time for the given sun altitude
    function getSetJ(h) {
        var w = getHourAngle(h, phi, dec),
            a = getApproxTransit(w, lw, n);

        return getSolarTransitJ(a, M, L);
    }


    var result = {
        solarNoon: fromJulian(Jnoon),
        nadir: fromJulian(Jnoon - 0.5)
    };

    var i, len, time, angle, morningName, eveningName, Jset, Jrise;

    for (i = 0, len = times.length; i < len; i += 1) {
        time = times[i];

        Jset = getSetJ(time[0] * rad);
        Jrise = Jnoon - (Jset - Jnoon);

        result[time[1]] = fromJulian(Jrise);
        result[time[2]] = fromJulian(Jset);
    }

    return result;
};


// moon calculations, based on http://aa.quae.nl/en/reken/hemelpositie.html formulas

function getMoonCoords(d) { // geocentric ecliptic coordinates of the moon

    var L = rad * (218.316 + 13.176396 * d), // ecliptic longitude
        M = rad * (134.963 + 13.064993 * d), // mean anomaly
        F = rad * (93.272 + 13.229350 * d), // mean distance

        l = L + rad * 6.289 * sin(M), // longitude
        b = rad * 5.128 * sin(F), // latitude
        dt = 385001 - 20905 * cos(M); // distance to the moon in km

    return {
        ra: getRightAscension(l, b),
        dec: getDeclination(l, b),
        dist: dt
    };
}

SunCalc.getMoonPosition = function (date, lat, lng) {

    var lw = rad * -lng,
        phi = rad * lat,
        d = toDays(date),

        c = getMoonCoords(d),
        H = getSiderealTime(d, lw) - c.ra,
        h = getAltitude(H, phi, c.dec);

    // altitude correction for refraction
    h = h + rad * 0.017 / tan(h + rad * 10.26 / (h + rad * 5.10));

    return {
        azimuth: getAzimuth(H, phi, c.dec),
        altitude: h,
        distance: c.dist
    };
};


// calculations for illumination parameters of the moon,
// based on http://idlastro.gsfc.nasa.gov/ftp/pro/astro/mphase.pro formulas and
// Chapter 48 of "Astronomical Algorithms" 2nd edition by Jean Meeus
// (Willmann-Bell, Richmond) 1998.

SunCalc.getMoonIllumination = function (date) {

    var d = toDays(date),
        s = getSunCoords(d),
        m = getMoonCoords(d),

        sdist = 149598000, // distance from Earth to Sun in km

        phi = acos(sin(s.dec) * sin(m.dec) + cos(s.dec) * cos(m.dec) * cos(s.ra - m.ra)),
        inc = atan(sdist * sin(phi), m.dist - sdist * cos(phi));

    return {
        fraction: (1 + cos(inc)) / 2,
        angle: atan(cos(s.dec) * sin(s.ra - m.ra), sin(s.dec) * cos(m.dec)
            - cos(s.dec) * sin(m.dec) * cos(s.ra - m.ra))
    };
};


// export as AMD module / Node module / browser variable

if (typeof define === 'function' && define.amd) {
    define(SunCalc);
} else if (typeof module !== 'undefined') {
    module.exports = SunCalc;
} else {
    window.SunCalc = SunCalc;
}

}());


/* Now, 155 lines of base64 decoding, to work around two issues:
 *  1)  Something in the e.response process is mangling 2-byte UTF8 characters into two 1-byte characters
 *  2)  Ejecta JSCore doesn't have an atob() function, so we must provide our own...
 *
 * Should Ejecta JSCore gain the atob() function, or the bug be fixed, I anticipate removing this.
 *
 *  ... leaving the encode functions in case I ever need to send, because, why not.
 */

/*
 * Copyright (c) 2010 Nick Galbreath
 * http://code.google.com/p/stringencoders/source/browse/#svn/trunk/javascript
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
*/

/* base64 encode/decode compatible with window.btoa/atob
 *
 * window.atob/btoa is a Firefox extension to convert binary data (the "b")
 * to base64 (ascii, the "a").
 *
 * It is also found in Safari and Chrome.  It is not available in IE.
 *
 * if (!window.btoa) window.btoa = base64.encode
 * if (!window.atob) window.atob = base64.decode
 *
 * The original spec's for atob/btoa are a bit lacking
 * https://developer.mozilla.org/en/DOM/window.atob
 * https://developer.mozilla.org/en/DOM/window.btoa
 *
 * window.btoa and base64.encode takes a string where charCodeAt is [0,255]
 * If any character is not [0,255], then an exception is thrown.
 *
 * window.atob and base64.decode take a base64-encoded string
 * If the input length is not a multiple of 4, or contains invalid characters
 *   then an exception is thrown.
 */
base64 = {};
base64.PADCHAR = '=';
base64.ALPHA = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
base64.getbyte64 = function(s,i) {
    // This is oddly fast, except on Chrome/V8.
    //  Minimal or no improvement in performance by using a
    //   object with properties mapping chars to value (eg. 'A': 0)
    var idx = base64.ALPHA.indexOf(s.charAt(i));
    if (idx == -1) {
        throw "Cannot decode base64";
    }
    return idx;
}

base64.decode = function(s) {
    // convert to string
    s = "" + s;
    var getbyte64 = base64.getbyte64;
    var pads, i, b10;
    var imax = s.length
    if (imax == 0) {
        return s;
    }

    if (imax % 4 != 0) {
        throw "Cannot decode base64";
    }

    pads = 0
    if (s.charAt(imax -1) == base64.PADCHAR) {
        pads = 1;
        if (s.charAt(imax -2) == base64.PADCHAR) {
            pads = 2;
        }
        // either way, we want to ignore this last block
        imax -= 4;
    }

    var x = [];
    for (i = 0; i < imax; i += 4) {
        b10 = (getbyte64(s,i) << 18) | (getbyte64(s,i+1) << 12) |
            (getbyte64(s,i+2) << 6) | getbyte64(s,i+3);
        x.push(String.fromCharCode(b10 >> 16, (b10 >> 8) & 0xff, b10 & 0xff));
    }

    switch (pads) {
    case 1:
        b10 = (getbyte64(s,i) << 18) | (getbyte64(s,i+1) << 12) | (getbyte64(s,i+2) << 6)
        x.push(String.fromCharCode(b10 >> 16, (b10 >> 8) & 0xff));
        break;
    case 2:
        b10 = (getbyte64(s,i) << 18) | (getbyte64(s,i+1) << 12);
        x.push(String.fromCharCode(b10 >> 16));
        break;
    }
    return x.join('');
}

base64.getbyte = function(s,i) {
    var x = s.charCodeAt(i);
    if (x > 255) {
        throw "INVALID_CHARACTER_ERR: DOM Exception 5";
    }
    return x;
}


base64.encode = function(s) {
    if (arguments.length != 1) {
        throw "SyntaxError: Not enough arguments";
    }
    var padchar = base64.PADCHAR;
    var alpha   = base64.ALPHA;
    var getbyte = base64.getbyte;

    var i, b10;
    var x = [];

    // convert to string
    s = "" + s;

    var imax = s.length - s.length % 3;

    if (s.length == 0) {
        return s;
    }
    for (i = 0; i < imax; i += 3) {
        b10 = (getbyte(s,i) << 16) | (getbyte(s,i+1) << 8) | getbyte(s,i+2);
        x.push(alpha.charAt(b10 >> 18));
        x.push(alpha.charAt((b10 >> 12) & 0x3F));
        x.push(alpha.charAt((b10 >> 6) & 0x3f));
        x.push(alpha.charAt(b10 & 0x3f));
    }
    switch (s.length - imax) {
    case 1:
        b10 = getbyte(s,i) << 16;
        x.push(alpha.charAt(b10 >> 18) + alpha.charAt((b10 >> 12) & 0x3F) +
               padchar + padchar);
        break;
    case 2:
        b10 = (getbyte(s,i) << 16) | (getbyte(s,i+1) << 8);
        x.push(alpha.charAt(b10 >> 18) + alpha.charAt((b10 >> 12) & 0x3F) +
               alpha.charAt((b10 >> 6) & 0x3f) + padchar);
        break;
    }
    return x.join('');
}
