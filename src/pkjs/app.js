//var hasTimeline = 1;
//var topic = "not_set";
var fix = 0;
//var defaultId = 99;
var logging = 1;
var mmol_MinLimit = 0.9;
var mgdl_MinLimit = 39;
// Error messages
var servererror = "E_E3"; // E3 - Server Error
var timeouterror = "E_E2"; // E2 - Time out error
var loginerror = "E_E1"; // E1 - Login Error
var dataerror = "E_E4"; // E4 - Data Error
// main function to retrieve, format, and send cgm data
function fetchCgmData() {
        console.log("START fetchCgmData 1");
        // declare local variables for message data
        var opts = [].slice.call(arguments).pop();
        opts = JSON.parse(localStorage.getItem('cgmPebble_new'));
        //console.log("-------- opts 1: " + opts);
       // switch (opts.mode) {
         //   case "Nightscout":
                console.log("Nightscout data to be loaded");
                //subscribeBy(opts.endpoint);
                opts.endpoint = opts.endpoint.replace("/pebble?units=mmol", "");
                opts.endpoint = opts.endpoint.replace("/pebble/", "");
                opts.endpoint = opts.endpoint.replace("/pebble", "");
                nightscout(opts);
                getLoopData(opts);
    } // end fetchCgmData
    //Get last 9 SGV's

function getNightScoutArrayData(message, opts) {
        // opts.vibe = parseInt(opts.vibe, 10);
        // console.log("OPTS:" + opts);
        var endpoint = opts.endpoint.replace("/pebble?units=mmol", "");
        endpoint = endpoint.replace("/pebble/", "");
        endpoint = endpoint.replace("/pebble", "");
        var url = endpoint + "/api/v1/entries/sgv.json?count=12";
        var http = new XMLHttpRequest();
        http.open("GET", url, false);
        http.onload = function(e) {
            if (http.status == 200) {
                var data = JSON.parse(http.responseText);
                // Add the array data for graph
                message.bgsx = createBgArray(data, opts);
                message.bgty = createBgTimeArray(data, opts);
                console.log("Data from treatments call:" + JSON.stringify(
                    message));
                return message;
            } else {
                sendUnknownError("dataerr");
            }
        };
        http.onerror = function() {
            sendServerError();
        };
        http.ontimeout = function() {
            sendTimeOutError();
        };
        try {
            http.send();
        } catch (e) {
            sendUnknownError("invalidurl");
        }
        //console.log("Message 4:" + JSON.stringify(message));
        return message;
    } //end
    //Create BG ARRAY

function createBgArray(data, opts) {
        var toReturn = "";
        //console.log("-----------------------data.length:" + data.length);
        var now = new Date();
        var minLimit = mmol_MinLimit; // mmol
        if (opts.radio == 'mgdl_form') {
            minLimit = mgdl_MinLimit;
        }
        for (var i = 0; i < data.length; i++) {
            var wall = parseInt(data[i].date);
            var timeAgo = msToMinutes(now.getTime() - wall);
            //console.log("----createBgArray------ data[i].type: " + data[i].type + "   data[i].sgv: " + data[i].sgv);
            if (timeAgo < 120 && data[i].type == 'sgv' && data[i].sgv >=
                minLimit) {
                toReturn = toReturn + data[i].sgv.toString() + ",";
            }
        }
        toReturn = toReturn.replace(/,\s*$/, "");
        return toReturn;
    } //end createBgTimeArray

function createBgTimeArray(data, opts) {
        //  var bgHour = "00";
        //  var bgMin = "00";
        var toReturn2 = "";
        var now = new Date();
        var minLimit = mmol_MinLimit; // mmol
        if (opts.radio == 'mgdl_form') {
            minLimit = mgdl_MinLimit;
        }
        for (var i = 0; i < data.length; i++) {
            var wall = parseInt(data[i].date);
            var timeAgo = msToMinutes(now.getTime() - wall);
            //console.log("-----createBgTimeArray----- data[i].type: " + data[i].type + "   data[i].sgv: " + data[i].sgv);
            if (timeAgo < 120 && data[i].type == 'sgv' && data[i].sgv >=
                minLimit) {
                toReturn2 = toReturn2 + (120 - timeAgo).toString() + ",";
            }
        }
        toReturn2 = toReturn2.replace(/,\s*$/, "");
        // console.log("createBgTimeArray: " + toReturn2);
        return toReturn2;
    } // end createBgTimeArray

function timeSince(date) {
    var seconds = Math.floor((new Date() - date) / 1000);
    //var seconds = Math.floor(((new Date().getTime()/1000) - date))
    var interval = Math.floor(seconds / 31536000);
    if (interval >= 1) {
        return interval + "y";
    }
    interval = Math.floor(seconds / 2592000);
    if (interval >= 1) {
        return interval + "m";
    }
    interval = Math.floor(seconds / 86400);
    if (interval >= 1) {
        return interval + "d";
    }
    interval = Math.floor(seconds / 3600);
    if (interval >= 1) {
        return interval + "h";
    }
    interval = Math.floor(seconds / 60);
    if (interval >= 1) {
        return interval + "m";
    }
    return Math.floor(seconds) + "s";
}
function getLoopData(opts) {
        var endpoint = opts.endpoint.replace("/pebble?units=mmol", "");
        endpoint = endpoint.replace("/pebble/", "");
        endpoint = endpoint.replace("/pebble", "");
        var loopurl = endpoint + "/api/v2/properties/iob,loop,basal,pump,openaps";
        var http = new XMLHttpRequest();
        http.open("GET", loopurl, false);
        http.onload = function(e) {
            if (http.status == 200) {
                //var loopdata = (http.responseText);
                var loopn = JSON.parse(http.responseText);
                if (loopn.length === 0) {
                    sendUnknownError("dataerr");
                    //return loopok;
                    // console.log("Typeof" typeof loopsymbol);
                } else {
                  if ((loopn['openaps'] ===undefined) || (loopn.openaps.lastEnacted === null)){
                    if ((loopn.loop.lastLoop.lastOkMoment === null)) {
                        opts.Last = "null";
                    } else {
                        opts.Last = loopn.loop.lastLoop.timestamp;
                    }
                    opts.Symbol = loopn.loop.display.label;
                   // var cob = loopn.loop.lastLoop.cob.cob;
                   // opts.Cob = (Math.round(cob).toFixed(1));
                   // console.log("optscob"+opts.Cob);
                  } else {
                    console.log("start openaps");
                    if ((loopn.openaps.lastLoopMoment === null)) {
                        opts.Last = "null";
                    } else {
                        opts.Last = loopn.openaps.lastLoopMoment;

                    }
                    opts.Symbol = loopn.openaps.status.label;

                  }
                  var d = new Date(opts.Last);
                  opts.LoopTime = timeSince(d);
                  console.log("opts.Last " +opts.Last + "opts.LoopTime" + opts.LoopTime);
                  if ((loopn.pump.pump === undefined)) {
                       opts.Pump = "No Data Available";
                  }else{
                  var pumpbat = loopn.pump.pump.battery.voltage;
                  var pumpres = (loopn.pump.pump.reservoir + "u");
                  var pumpdat = loopn.pump.data.clock.display;
                    
                  opts.Pump = ("Bat:" + pumpbat + "v" + " " + "Res:" + pumpres + " " + pumpdat);
                  }
                  /*if ((loopn.iob.display === null)) {
                      opts.iob = "null";
                    }else{
                    opts.iob = Math.round(loopn.iob.iob).toFixed(1);
                    }*/
                  console.log ("pump info " + opts.Pump);
                    opts.Basal = loopn.basal.display;
                    console.log("loop body: " + " " + opts.LoopTime + " " + opts.Symbol + " " + opts.Basal);
                }
            }
        };
        http.onerror = function() {
            sendServerError();
        };
        http.ontimeout = function() {
            sendTimeOutError();
        };
        try {
            http.send();
        } catch (e) {
            sendUnknownError("invalidurl");
        }
       
    } //end GETLOOPDATA
    
//get icon, bg, timeago, delta, name, noiz, raw, options

function nightscout(opts) {
    //var loopok = loopn.loop.lastOkMoment;
    // getLoopData();
    opts.endpoint = opts.endpoint + "/pebble";
    //console.log ("START fetchCgmData");
    // declare local variables for message data
    var response, responsebgs, responsecals, message;
    // check if endpoint exists
    if (!opts.endpoint) {
        // endpoint doesn't exist, return no endpoint to watch
        // " " (space) shows these are init values, not bad or null values
        message = {
            icon: " ",
            bg: " ",
            tcgm: 0,
            tapp: 0,
            dlta: "NOEP",
            ubat: " ",
            name: " ",
            vals: " ",
            bgsx: " ",
            bgty: " ",
            clrw: " ",
            cob: " ",
            sym: " ",
            time: " ",
            basal: " ",
            pump:  " ",

        };
        console.log("NO ENDPOINT JS message", JSON.stringify(message));
        MessageQueue.sendAppMessage(message);
        return;
    } // if (!opts.endpoint)
    // show current options
    //console.log("fetchCgmData IN OPTIONS = " + JSON.stringify(opts));
    // call XML
    var req = new XMLHttpRequest();
    // set timeout function June 23
    var myCGMTimeout = setTimeout(function() {
        req.abort();
        message = {
            dlta: "OFF"
        };
        console.log("TIMEOUT, DATA OFFLINE JS message", JSON.stringify(
            message));
        MessageQueue.sendAppMessage(message);
    }, 59000); // timeout in ms; set at 59 seconds; can not go beyond 59 seconds
    // get cgm data
    var arraydata = {};
    arraydata = getNightScoutArrayData(arraydata, opts);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if (req.status == 200) {
                // clear the XML timeout
                clearTimeout(myCGMTimeout);
                // Load response
                console.log(req.responseText);
                response = JSON.parse(req.responseText);
                responsebgs = response.bgs;
                responsecals = response.cals;
                //ADDED NEW
                // check response data
                if (responsebgs && responsebgs.length > 0) {
                    // response data is good; send log with response
                    // console.log('got response', JSON.stringify(response));
                    // initialize message data
                    // get direction arrow and BG
                    var currentDirection = responsebgs[0].direction,
                        values = " ",
                        currentIcon = "10",
                        currentBG = responsebgs[0].sgv,
                        currentConvBG = currentBG,
                        rawCalcOffset = 5,
                        specialValue = false,
                        calibrationValue = false,
                        // get CGM time delta and format
                        readingTime = new Date(responsebgs[0].datetime).getTime(),
                        //readingTime = null,
                        formatReadTime = Math.floor((readingTime / 1000)),
                        // get app time and format
                        appTime = new Date().getTime(),
                        //appTime = null,
                        formatAppTime = Math.floor((appTime / 1000)),
                        // get BG delta and format
                        currentBGDelta = responsebgs[0].bgdelta,
                        //currentBGDelta = -8,
                        formatBGDelta = " ",
                        //get loop information'
                        loopSym = opts.Symbol,
                        loopBasal = opts.Basal,
                        loopLast = opts.LoopTime,
                        currentSym = loopSym,
                        // get battery level
                        currentBattery = responsebgs[0].battery,
                        //currentBattery = "100",
                        // get NameofT1DPerson and IOB
                        NameofT1DPerson = opts.t1name,
                        currentIOB = responsebgs[0].iob,
                        // sensor fields
                        currentCalcRaw = 0,
                        //currentCalcRaw = 100000,
                        formatCalcRaw = " ",
                        currentRawFilt = responsebgs[0].filtered,
                        formatRawFilt = " ",
                        currentRawUnfilt = responsebgs[0].unfiltered,
                        formatRawUnfilt = " ",
                        //currentNoise = responsebgs[0].noise,
                        currentCOB = responsebgs[0].cob,
                        currentIntercept = "undefined",
                        currentSlope = "undefined",
                        currentScale = "undefined",
                        currentRatio = 0;
                   // get name of T1D; if iob (case insensitive), use IOB
                    if ((NameofT1DPerson.toUpperCase() === "IOB") && ((typeof currentIOB != "undefined") && (currentIOB !== null))) {
                        // NameofT1DPerson = currentIOB + "u";
                        NameofT1DPerson = currentIOB;
                    } else {
                        NameofT1DPerson = opts.t1name;
                    }
                  console.log ("IOB " + currentIOB + "NAME "+ NameofT1DPerson);
                    if (responsecals && responsecals.length > 0) {
                        currentIntercept = responsecals[0].intercept;
                        currentSlope = responsecals[0].slope;
                        currentScale = responsecals[0].scale;
                    }
                    //currentDirection = "NONE";
                    // set some specific flags needed for later
                    if (opts.radio == "mgdl_form") {
                        if ((currentBG < 40) || (currentBG > 400)) {
                            specialValue = true;
                        }
                        if (currentBG == 5) {
                            calibrationValue = true;
                        }
                    } else {
                        if ((currentBG < 2.3) || (currentBG > 22.2)) {
                            specialValue = true;
                        }
                        if (currentBG == 0.3) {
                            calibrationValue = true;
                        }
                        currentConvBG = (Math.round(currentBG * 18.018)
                            .toFixed(0));
                    }
                    // convert arrow to a number string; sending number string to save memory
                    // putting NOT COMPUTABLE first because that's most common and can get out fastest
                    switch (currentDirection) {
                        case "NOT COMPUTABLE":
                            currentIcon = "8";
                            break;
                        case "NOT_COMPUTABLE":
                            currentIcon = "8";
                            break;
                        case "NONE":
                            currentIcon = "0";
                            break;
                        case "DoubleUp":
                            currentIcon = "1";
                            break;
                        case "SingleUp":
                            currentIcon = "2";
                            break;
                        case "FortyFiveUp":
                            currentIcon = "3";
                            break;
                        case "Flat":
                            currentIcon = "4";
                            break;
                        case "FortyFiveDown":
                            currentIcon = "5";
                            break;
                        case "SingleDown":
                            currentIcon = "6";
                            break;
                        case "DoubleDown":
                            currentIcon = "7";
                            break;
                        case "RATE OUT OF RANGE":
                            currentIcon = "9";
                            break;
                        default:
                            currentIcon = "10";
                    }
                    // if no battery being sent yet, then send nothing to watch
                    // console.log("Battery Value: " + currentBattery);
                    if ((typeof currentBattery == "undefined") || (
                        currentBattery === null)) {
                        currentBattery = " ";
                    }
                    //no cob, nothing to watch
                    if ((typeof currentCOB == "undefined") || (
                        currentCOB === null)) {
                        currentCOB = " ";
                    }
                    currentCOB = (currentCOB.toString());
                    // assign bg delta string
                    formatBGDelta = ((currentBGDelta > 0 ? '+' : '') +
                        currentBGDelta);
                    //console.log("Current Unfiltered: " + currentRawUnfilt);
                    //console.log("Current Intercept: " + currentIntercept);
                    //console.log("Special Value Flag: " + specialValue);
                    //console.log("Current BG: " + currentBG);
                    // assign calculated raw value if we can
                    if ((typeof currentIntercept != "undefined") && (
                        currentIntercept !== null)) {
                        if (specialValue) {
                            // don't use ratio adjustment
                            currentCalcRaw = ((currentScale * (
                                        currentRawUnfilt -
                                        currentIntercept) /
                                    currentSlope) * 1 -
                                rawCalcOffset * 1);
                            //console.log("Special Value Calculated Raw: " + currentCalcRaw);
                        } else {
                            currentRatio = (currentScale * (
                                    currentRawFilt -
                                    currentIntercept) /
                                currentSlope / (currentConvBG * 1 +
                                    rawCalcOffset * 1));
                            currentCalcRaw = ((currentScale * (
                                        currentRawUnfilt -
                                        currentIntercept) /
                                    currentSlope / currentRatio) *
                                1 - rawCalcOffset * 1);
                            //console.log("Current Converted BG: " + currentConvBG);
                            //console.log("Current Ratio: " + currentRatio);
                            //console.log("Normal BG Calculated Raw: " + currentCalcRaw);
                        }
                    } // if currentIntercept
                    // assign raw sensor values if they exist
                    if ((typeof currentRawUnfilt != "undefined") && (
                        currentRawUnfilt !== null)) {
                        // zero out any invalid values; defined anything not between 0 and 900
                        if ((currentRawFilt < 0) || (currentRawFilt >
                            900000) || (isNaN(currentRawFilt))) {
                            currentRawFilt = "ERR";
                        }
                        //if ((currentRawUnfilt < 0) || (currentRawUnfilt > 900000) ||
                        //    (isNaN(currentRawUnfilt))) {
                        //    currentRawUnfilt = "ERR";
                        // }
                        // set 0, LO and HI in calculated raw
                        if ((currentCalcRaw >= 0) && (currentCalcRaw <
                            30)) {
                            formatCalcRaw = "LO";
                        }
                        if ((currentCalcRaw > 500) && (currentCalcRaw <=
                            900)) {
                            formatCalcRaw = "HI";
                        }
                        if ((currentCalcRaw < 0) || (currentCalcRaw >
                            900)) {
                            formatCalcRaw = "ERR";
                        }
                        // if slope is 0 or if currentCalcRaw is NaN,
                        // calculated raw is invalid and need a calibration
                        if ((currentSlope === 0) || (isNaN(
                            currentCalcRaw))) {
                            formatCalcRaw = "CAL";
                        }
                        // check for compression warning
                        if (((currentCalcRaw < (currentRawFilt / 1000)) &&
                            (!calibrationValue)) && (currentRawFilt !==
                            0)) {
                            var compressionSlope = 0;
                            compressionSlope = (((currentRawFilt / 1000) -
                                currentCalcRaw) / (
                                currentRawFilt / 1000));
                            //console.log("compression slope: " + compressionSlope);
                            if (compressionSlope > 0.7) {
                                // set COMPRESSION? message
                                formatBGDelta = "PRSS";
                            } // if compressionSlope
                        } // if check for compression condition
                        if (opts.radio == "mgdl_form") {
                            formatRawFilt = ((Math.round(currentRawFilt /
                                1000)).toFixed(0));
                            formatRawUnfilt = ((Math.round(
                                currentRawUnfilt / 1000)).toFixed(
                                0));
                            if ((formatCalcRaw != "LO") && (
                                formatCalcRaw != "HI") && (
                                formatCalcRaw != "ERR") && (
                                formatCalcRaw != "CAL")) {
                                formatCalcRaw = ((Math.round(
                                    currentCalcRaw)).toFixed(0));
                            }
                            //console.log("Format Unfiltered: " + formatRawUnfilt);
                        } else {
                            formatRawFilt = ((Math.round(((
                                currentRawFilt /
                                1000) * 0.0555) * 10) / 10).toFixed(
                                1));
                            formatRawUnfilt = ((Math.round(((
                                currentRawUnfilt /
                                1000) * 0.0555) * 10) / 10).toFixed(
                                1));
                            if ((formatCalcRaw != "LO") && (
                                formatCalcRaw != "HI") && (
                                formatCalcRaw != "ERR") && (
                                formatCalcRaw != "CAL")) {
                                formatCalcRaw = ((Math.round(
                                    currentCalcRaw) * 0.0555).toFixed(
                                    1));
                            }
                            //console.log("Format Unfiltered: " + formatRawUnfilt);
                        }
                    } // if currentRawUnfilt
                    
                  //add raw to pump shake data
                  var loopPump;
                  if (formatCalcRaw === null){
                    loopPump = opts.Pump;
                  } else {
                    loopPump = " Raw:" + formatCalcRaw  + " " + opts.Pump ;
                  }

                    if (opts.radio == "mgdl_form") {
                        values = "0"; //mgdl selected
                    } else {
                        values = "1"; //mmol selected
                    }
                    values += "," + opts.lowbg; //Low BG Level
                    values += "," + opts.highbg; //High BG Level
                    values += "," + opts.lowsnooze; //LowSnooze minutes
                    values += "," + opts.highsnooze; //HighSnooze minutes
                    values += "," + opts.lowvibe; //Low Vibration
                    values += "," + opts.highvibe; //High Vibration
                    values += "," + opts.vibepattern; //Vibration Pattern
                    if (opts.timeformat == "12") {
                        values += ",0"; //Time Format 12 Hour
                    } else {
                        values += ",1"; //Time Format 24 Hour
                    }
                    // Vibrate on raw value in special value; Yes = 1; No = 0;
                    if ((currentCalcRaw !== 0) && (opts.rawvibrate ==
                        "1")) {
                        values += ",1"; // Vibrate on raw value when in special values
                    } else {
                        values += ",0"; // Do not vibrate on raw value when in special values
                    }
                    values += "," + opts.mycolors; // Color field
                    values += "," + opts.animateon; // Animation ON or OFF
                    values += "," + opts.vibeon;  // Vibrate on or off
                  //loop symbol
                  switch (loopSym) {
                        case "Enacted":
                            currentSym = "E";
                            break;
                        case "Looping":
                            currentSym = "L";
                            break;
                        case "Error":
                            currentSym = "X";
                            break;
                        case "Recomendation":
                            currentSym = "R";
                            break;
                        case "Warning":
                           currentSym = "W";
                            break;
                        default:
                            currentSym = " ";
                            console.log("CurrentSym" + currentSym);
                    }
                    //var mode_switch = getModeAsInteger(opts);
                    // load message data
                    message = {
                        icon: currentIcon,
                        bg: currentBG,
                        tcgm: formatReadTime,
                        tapp: formatAppTime,
                        dlta: formatBGDelta,
                        ubat: currentBattery,
                        name: NameofT1DPerson,
                        vals: values,
                        clrw: formatCalcRaw,
                        cob: currentCOB,
                        bgsx: arraydata.bgsx,
                        bgty: arraydata.bgty,
                        sym: currentSym,
                        time: loopLast,
                        basal: loopBasal,
                        pump: loopPump,

                    };
                    // send message data to log and to watch
                    console.log("JS send message: " + JSON.stringify(
                        message));
                    MessageQueue.sendAppMessage(message);
                    // response data is not good; format error message and send to watch
                    // have to send space in BG field for logo to show up on screen
                } else {
                    // " " (space) shows these are init values (even though it's an error), not bad or null values
                    message = {
                        dlta: "OFF"
                    };
                    console.log("DATA OFFLINE JS message", JSON.stringify(
                        message));
                    MessageQueue.sendAppMessage(message);
                }
            } // end req.status == 200
        } // end req.readyState == 4
    }; // req.onload
    req.onerror = function(e) {
        console.log("XMLHttpRequest error: " + req.statusText);
    }; // end req.onerror
    // set rest of req
    req.open('GET', opts.endpoint, true);
    req.setRequestHeader('Cache-Control', 'no-cache');
    req.timeout = 59000; // Set timeout to 59 seconds (59000 milliseconds); can not go beyond 59 seconds
    req.ontimeout = myCGMTimeout;
    req.send(null);
    //var myCGMTimeout = setTimeout(function() {
    //req.abort();
    //message = {
    //     dlta: "OFF"
    // };
    //  console.log("DATA OFFLINE JS message", JSON.stringify(message));
    //  MessageQueue.sendAppMessage(message);
    //}, 59000); // timeout in ms; set at 45 seconds; can not go beyond 59 seconds
}
function msToMinutes(millisec) {
    return (millisec / (1000 * 60)).toFixed(1);
}
function sendAuthError() {
    console.log("===============ERROR: sendAuthError");
    Pebble.sendAppMessage({
        //"vibe": 1,
        "bg": " ",
        "icon": 0,
        // "alert": 4,
        "dlta": "LOG ERR",
        //  "id": defaultId,
        // "tcgm": defaultId,
        // "time_delta_int": -1,
    });
}

function sendTimeOutError(options) {
    console.log("===============ERROR: sendTimeOutError: " + JSON.stringify(
        options));
    Pebble.sendAppMessage({
        "bg": " ",
        "icon": 0,
        // "alert": 4,
        "dlta": "T-OUT",
        //  "id": defaultId,
        // "tcgm": defaultId,
        // "time_delta_int": -1,
    });
}

function sendServerError(options) {
    console.log("===============ERROR: sendServerError");
    Pebble.sendAppMessage({
        "bg": " ",
        "icon": 0,
        // "alert": 4,
        "dlta": "SVR ERR",
        //  "id": defaultId,
        // "tcgm": defaultId,
        // "time_delta_int": -1,
    });
}

function sendUnknownError(msg) {
    console.log("===============ERROR: sendUnknownError: " + msg);
    Pebble.sendAppMessage({
        "bg": " ",
        "icon": 0,
        // "alert": 4,
        "dlta": "ERR",
        //  "id": defaultId,
        // "tcgm": defaultId,
        // "time_delta_int": -1,
    });
}
function logging(message) {
        if (logging) console.log(message);
    }
    // message queue-ing to pace calls from C function on watch
var MessageQueue = (function() {
    var RETRY_MAX = 5;
    var queue = [];
    var sending = false;
    var timer = null;
    return {
        reset: reset,
        sendAppMessage: sendAppMessage,
        size: size
    };

    function reset() {
        queue = [];
        sending = false;
    }

    function sendAppMessage(message, ack, nack) {
        if (!isValidMessage(message)) {
            return false;
        }
        queue.push({
            message: message,
            ack: ack || null,
            nack: nack || null,
            attempts: 0
        });
        setTimeout(function() {
            sendNextMessage();
        }, 2);
        return true;
    }

    function size() {
        return queue.length;
    }

    function isValidMessage(message) {
        // A message must be an object.
        if (message !== Object(message)) {
            return false;
        }
        var keys = Object.keys(message);
        // A message must have at least one key.
        if (!keys.length) {
            return false;
        }
        for (var k = 0; k < keys.length; k += 1) {
            var validKey = /^[0-9a-zA-Z-_]*$/.test(keys[k]);
            if (!validKey) {
                return false;
            }
            var value = message[keys[k]];
            if (!validValue(value)) {
                return false;
            }
        }
        return true;

        function validValue(value) {
            switch (typeof value) {
                case 'string':
                    return true;
                case 'number':
                    return true;
                case 'object':
                    if (toString.call(value) ===
                        '[object Array]') {
                        return true;
                    }
            }
            return false;
        }
    }

    function sendNextMessage() {
        if (sending) {
            return;
        }
        var message = queue.shift();
        if (!message) {
            return;
        }
        message.attempts += 1;
        sending = true;
        Pebble.sendAppMessage(message.message, ack, nack);
        timer = setTimeout(function() {
            timeout();
        }, 2000);

        function ack() {
            clearTimeout(timer);
            setTimeout(function() {
                sending = false;
                sendNextMessage();
            }, 400);
            if (message.ack) {
                message.ack.apply(null, arguments);
            }
        }

        function nack() {
            clearTimeout(timer);
            if (message.attempts < RETRY_MAX) {
                queue.unshift(message);
                setTimeout(function() {
                    sending = false;
                    sendNextMessage();
                }, 400 * message.attempts);
            } else {
                if (message.nack) {
                    message.nack.apply(null, arguments);
                }
            }
        }

        function timeout() {
            setTimeout(function() {
                sending = false;
                sendNextMessage();
            }, 2000);
            if (message.ack) {
                message.ack.apply(null, arguments);
            }
        }
    }
}());
// pebble specific calls with watch
var s = 0;
setInterval(function() {
    s++;
    console.log("JavaScript is running for " + s + " seconds.");
}, 1000);

Pebble.addEventListener("appmessage", function(e) {
    console.log("JS Recvd Msg From Watch: " + JSON.stringify(e.payload));
    fetchCgmData();
});
Pebble.addEventListener("showConfiguration", function(e) {
    console.log("Showing Configuration", JSON.stringify(e));
    Pebble.openURL(
        'http://cgminthecloud.github.io/CGMClassicPebble/skylineloop.html'
    );
});
Pebble.addEventListener("webviewclosed", function(e) {
    var opts = JSON.parse(decodeURIComponent(e.response));
    console.log("CLOSE CONFIG OPTIONS = " + JSON.stringify(opts));
    // store endpoint in local storage
    localStorage.setItem('cgmPebble_new', JSON.stringify(opts));
});
