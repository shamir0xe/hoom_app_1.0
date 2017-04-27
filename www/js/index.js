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


$(function () {
    var selected = null;
    const membersLimit = 99;

    $('#alert-1').delay(2000).slideUp(1000);

    function stylingSelected(selectedId) {
        $($('#list-button').children()[0]).html(names[selectedId]);
        console.log('changed the list name to ', names[selectedId]);
    }

    {
        var unorderedList = $('#members-list');
        for (var key in names) {

            var pElement = $(document.createElement('p')).html(names[key]).attr('class', 'persian-text nopadding text-center list-item');
            var aElement = $(document.createElement('a')).attr('href', '#');
            aElement.append(pElement);
            var liElement = $(document.createElement('li')).attr('id', 'list-item-#' + key);
            liElement.append(aElement);
            if (parseInt(key) < membersLimit) {
                unorderedList.prepend(liElement);
            } else {
                unorderedList.append(liElement);
            }
        }
    }

    $('#go-home-btn').on('click', function () {
        $.getJSON(hostAddress, null, function(data) {
            console.log('sent!');
            console.log(data);
        });
    });

    $(document).on('click', '.list-item', function () {
        var id = $(this).parent().parent().attr('id');
        console.log('selected id is: ', id);
        id = id.substr(id.indexOf('#') + 1);
        console.log('selected id is: ', id);
        selected = id;
        stylingSelected(id);
    });
});

