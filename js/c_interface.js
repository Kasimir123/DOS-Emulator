let read_success = null;
let open_success = null;
let get_char_success = null;
let get_char_b = false;
let video_mode = false;
let cur_key = '\x00';

const fileSelector = document.getElementById('file-selector');
fileSelector.addEventListener('change', (event) => {
    const fileList = event.target.files;
    var fileReader = new FileReader();
    fileReader.onload = function (event) {
        var arrayBuffer = event.target.result;
        var bytes = new Uint8Array(arrayBuffer);

        open_success(bytes.toString());
    }
    fileReader.readAsArrayBuffer(fileList[0]);
});


function read(success) {
    read_success = success;

};


function open_file(success) {
    open_success = success;
};

function get_char(success) {
    get_char_success = success;
    get_char_b = true;
}

function get_char_now(success) {
    success(cur_key);
}

document.addEventListener('keydown', (e) => {
    e = e || window.event;

    cur_key = e.key;

}, false);


document.addEventListener('keyup', (e) => {
    e = e || window.event;

    cur_key = '\x00';

    if (get_char_b) {

        if (e.key.length == 1) {
            get_char_success(e.key);
            get_char_b = false;
        }
        return true;
    }


    if (e.key == 'Enter' && $("#input").val().length > 0) {
        read_success($("#input").val());
        $("#input").val("");
        $("#output").val($("#output").val() + "\n");
        return false;
    }
    else if (e.key == "Backspace") {

        let input_val = $("#input").val();
        if (input_val.length > 0) {
            $("#input").val(input_val.substring(0, input_val.length - 1));
            $("#output").val($("#output").val().substring(0, $("#output").val().length - 1));

        }
    }
    else {
        if (e.key.length == 1) {
            $("#input").val($("#input").val() + e.key);
            $("#output").val($("#output").val() + e.key);
        }
    }
    return true;
}, false);


function emscripten_sleep(ms) {
    Asyncify.handleSleep(function (wakeUp) {
        setTimeout(wakeUp, ms);
    });
}

var re = /^___emulator::/;
// XHR proxy that handle methods from fetch in C
window.XMLHttpRequest = (function (xhr) {
    return function () {
        var url;
        var props = {
            readyState: 4,
            status: 200
        };
        var enc = new TextEncoder("utf-8");
        return new Proxy(new xhr(), {
            get: function (target, name) {
                if (url && ['response', 'responseText', 'status', 'readyState'].indexOf(name) != -1) {
                    if (name == 'response') {
                        var response = enc.encode(props.responseText);
                        return response;
                    }
                    return props[name];
                } else if (name == 'open') {
                    return function (method, open_url) {
                        if (open_url.match(re)) {
                            url = open_url;
                        } else {
                            return target[name].apply(target, arguments);
                        }
                    };
                } else if (name == 'send') {
                    return function (data) {
                        if (url) {
                            var payload = url.split('::');
                            if (payload[1] == 'read') {
                                $("#output").val($("#output").val() + "> ");
                                read(
                                    function (text) {
                                        props.responseText = text + '\0';
                                        target.onload();
                                    }
                                );
                            }
                            else if (payload[1] == 'open_file') {
                                $("#file-selector").show();
                                $("#output").val($("#output").val() + "Please Upload a File\n");
                                open_file(
                                    function (text) {
                                        $("#file-selector").hide();
                                        props.responseText = text;
                                        target.onload();
                                    }
                                );
                            }
                            else if (payload[1] == 'get_char') {
                                console.log("get_char");
                                get_char(
                                    function (text) {
                                        props.responseText = text + '\0';
                                        target.onload();
                                    }
                                );
                            }
                            else if (payload[1] == 'get_char_now') {
                                console.log("get_char_now");
                                get_char_now(
                                    function (text) {
                                        props.responseText = text + '\0';
                                        target.onload();
                                    }
                                );
                            }
                            else if (payload[1] == 'write') {
                                if (video_mode) {
                                    context.font = "20px Comic Sans MS";
                                    context.fillStyle = "red";
                                    context.fillText(payload[2], Number(payload[3]) * 15, Number(payload[4]) * 10);
                                }
                                else {
                                    $("#program_output").val($("#program_output").val() + payload[2]);
                                }

                            }
                            else if (payload[1] == 'activate_video_mode') {
                                video_mode = true;
                                console.log("video mode enabled");
                                $("#viewport").show();
                                $("#program_output").hide();

                            }
                            else if (payload[1] == 'start') {
                                video_mode = false;
                                console.log("program started");
                                $("#program_output").val("Program Output:\n");
                                $("#viewport").hide();
                                $("#program_output").show();

                                for (let i = 0; i < w; i++) {
                                    for (let j = 0; j < h; j++) {
                                        drawPixel(imagedata, i, j, colors[0xf]);
                                    }
                                }
                                context.putImageData(imagedata, 0, 0);

                            }
                            else if (payload[1] == 'set_background_color') {
                                for (let i = 0; i < w; i++) {
                                    for (let j = 0; j < h; j++) {
                                        drawPixel(imagedata, i, j, colors[Number(payload[2])]);
                                    }
                                }
                                context.putImageData(imagedata, 0, 0);

                            }
                            else if (payload[1] == 'draw_pixel') {
                                let col = Number(payload[3]) * 2;
                                let row = Number(payload[4]) * 2;

                                for (let i = 0; i < 2; i++)
                                    for (let j = 0; j < 2; j++)
                                        drawPixel(imagedata, col + i, row + j, colors[Number(payload[2])]);

                                context.putImageData(imagedata, 0, 0);
                            }

                        } else {
                            return target[name].apply(target, arguments);
                        }
                    };
                }
                return target[name];
            },
            set: function (target, name, value) {
                target[name] = value;
            }
        });
    };
})(window.XMLHttpRequest);