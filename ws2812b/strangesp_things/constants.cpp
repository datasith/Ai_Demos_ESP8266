#include "constants.h"
namespace constants 
{
  // Replace with your network credentials
  const char* ssid = "YOUR_SSID";
  const char* password = "YOUR_PASSWORD";

  const char webpage[] PROGMEM = R"=====(
    <html>
    <head>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
    h2 {
      line-height: 1;
      font-weight: 700;
      font-size: 48px;
      font-family: Arial, Helvetica, sans-serif;
      color: white;
    }
    .main{
      width:100%;
      position: relative;
      top: 50%;
      transform: translateY(-50%);
    }
    .container {
      background-color:rgba(72,72,72,0.4);
      padding-left:35px;
      padding-right:35px;
      padding-top:70px;
      padding-bottom:70px;
      text-align: center;
    }
    input {
      vertical-align: middle;
    }
    input[type="text"] {
      color:#3c3c3c;
      font-family: Helvetica, Arial, sans-serif;
      font-weight:500;
      font-size: 18px;
      background-color: #fbfbfb;
      padding: 13px;
      -webkit-box-sizing: border-box;
      -moz-box-sizing: border-box;
      -ms-box-sizing: border-box;
      box-sizing: border-box;
      border: 3px solid rgba(0,0,0,0);
      width: 50%;
    }
    input[type="button"]{
      font-family: Arial, Helvetica, sans-serif;
      border: #fbfbfb solid 4px;
      cursor:pointer;
      background-color: #3498db;
      color:white;
      font-size:24px;
      padding:8px 35px;
      -webkit-transition: all 0.3s;
      -moz-transition: all 0.3s;
      transition: all 0.3s;
      font-weight:700;
    }
    input:hover,
    input:focus {
      background-color:white;
      color: #3498db;
    }
    </style>
    </head>
    <body>
    <div class="main">
      <div class="container">
        <h2>Upside-Down Teletype</h2>
        <div>
          <input type="text" id="message" class="validate[required,custom[onlyLetter],length[0,100]] input" placeholder="Enter message..." />
          <input type="button" onclick="sendString()" value="SEND" />
        </div>
      </div>
    </div>
    <script>
      function sendString() {
        var msg = document.getElementById("message").value;
        var xhr = new XMLHttpRequest();

        xhr.open("POST","http://strangesp-things.local/process");
        xhr.setRequestHeader("Content-Type","application/json;charset=UTF-8");
        xhr.send(JSON.stringify({message:msg}));
      }
    </script>
    </body>
    </html>
  )=====";
}
