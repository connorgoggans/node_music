"use strict";

//Library imports
let player = require('play-sound')();
let express = require('express');
let fs = require('fs');
let logger = require('morgan');
let path = require('path');
let Mplayer = require('node-mplayer');

//Init express application
let app = express();

//Log requests coming in
app.use(logger('dev'));

//Start listening on port 1234
let server = app.listen(1234, () => {
    console.log('listening');
});

//Read from the music folder to see available songs
let songList = fs.readdirSync('music');
//console.log("Songs: " + songList);

//init status which keeps track of state in the application
let status = {
    audio: undefined,
    playing: false,
    curSong: 0
}

//default endpoint for serving the webpage
app.get('/', function (req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
});

//Just plays the current song in the queue
app.post('/play', function (req, res) {
    //create an audio object and save it to the state
    /*
    status["audio"] = player.play("music/" + songList[status["curSong"]], { mplayer: ['-ao', 'alsa:device=hw=1'] }, (err) => {
        if (err) throw err
    });

    status["playing"] = true;
    res.status(200).send(songList[status["curSong"]]);
    */
    //status["audio"] = new mplayer("music/" + songList[status["curSong"]]);
    let player = new Mplayer("music/" + songList[status["curSong"]]);
    player.play();
    player.getVolume(function(currentVolume){
        console.log(currentVolume);
    });
    //status['playing'] = true;
    res.status(200).send(songList[status["curSong"]]);
});


//advances playback by one song
app.post('/next', function (req, res) {
    //kill current song if it's playing
    if (status["playing"]) {
        status["audio"].kill();
    }

    //find the next song in the queue
    let nextSong = status['curSong'] + 1;
    if (nextSong >= songList.length) {
        nextSong = 0;
    }

    //play the new song and save state
    status["audio"] = player.play("music/" + songList[nextSong], { mplayer: ['-ao', 'alsa:device=hw=1'] }, (err) => {
        if (err) throw err
    });

    //save other states
    status["playing"] = true;
    status['curSong'] = nextSong;

    res.status(200).send(songList[nextSong]);
});

//reverses playback by one song
app.post('/previous', function (req, res) {
    //kill current song if it's playing
    if (status["playing"]) {
        status["audio"].kill();
    }

    //find the next song in the queue
    let nextSong = status['curSong'] - 1;
    if (nextSong < 0) {
        nextSong = songList.length - 1;
    }

    //play the new song and save state
    status["audio"] = player.play("music/" + songList[nextSong], { mplayer: ['-ao', 'alsa:device=hw=1'] }, (err) => {
        if (err) throw err
    });

    //save other states
    status["playing"] = true;
    status['curSong'] = nextSong;

    res.status(200).send(songList[nextSong]);
});

//kill the current song
app.post('/pause', function (req, res) {
    //if something is playing, stop it
    if (status["playing"]) {
        status["audio"].kill();
        status["playing"] = false;
    }
    res.sendStatus(200);
});

//return the song queue
app.get('/songs', function (req, res) {
    res.status(200).send(songList);
});