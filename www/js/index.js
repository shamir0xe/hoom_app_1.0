/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
var app = {
    // Application Constructor
    initialize: function() {
        document.addEventListener('deviceready', this.onDeviceReady.bind(this), false);
    },

    // deviceready Event Handler
    //
    // Bind any cordova events here. Common events are:
    // 'pause', 'resume', etc.
    onDeviceReady: function() {
        this.receivedEvent('deviceready');
    },

    // Update DOM on a Received Event
    receivedEvent: function(id) {
        var parentElement = document.getElementById(id);
        var listeningElement = parentElement.querySelector('.listening');
        var receivedElement = parentElement.querySelector('.received');

        listeningElement.setAttribute('style', 'display:none;');
        receivedElement.setAttribute('style', 'display:block;');

        console.log('Received Event: ' + id);
    }
};

app.initialize();
var hostAddress = "http://192.168.0.57/led?params=1";
var names = {
    "1": "ژاله",
    "2": "حمیدرضا",
    "3": "امیرحسین",
    "100": "admin"
};

{
    function selectedStyling(selected) {
        var element = document.getElementById('list-button');
        var paragraph = element.childNodes[1];
        paragraph.innerHTML = names[selected];
        console.log("changed the name to: ", names[selected]);
    }
    var selected = null;
    var listClickedFunction = function(e) {
        var element = (e.target).parentNode.parentNode;
        var id = element.getAttribute('id');
        console.log('selected id is:', id);
        id = id.substr(id.indexOf('#') + 1);
        console.log('selected id is:', id);
        selected = id;
        selectedStyling(selected);
    };
    $(document).on('click', '.list-item', listClickedFunction);
}

{
    var element = document.getElementById('go-home-btn');
    element.addEventListener('click', function () {
        $.getJSON(hostAddress, null, function(data) {
            console.log('sent!');
            console.log(data);
        });
    })
}
