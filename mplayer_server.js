"use strict";

//Library imports
let player = require('play-sound')();
let express = require('express');
let fs = require('fs');
let logger = require('morgan');
let path = require('path');
let Mplayer = require('mplayer');

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
    curSong: 0,
    volume: 50
}

//default endpoint for serving the webpage
app.get('/', function (req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
});

//Just plays the current song in the queue
app.post('/play', function (req, res) {
    //create an audio object and save it to the state
    if(status['audio']){
        status['audio'].play();
    }else{
        let player = new Mplayer();
        player.openFile("music/" + songList[status["curSong"]]);
        player.play();
        status['audio'] = player;
        status['playing'] = true;
        status['volume'] = 50;
    }
    
    res.status(200).send(songList[status["curSong"]]);
});

app.post('/ivolume', function(req, res){
    if(status['playing']){
        let volume = status['volume'];
        let newVolume = volume + 5;
        if(newVolume >= 100){
            status['audio'].volume(100);
            status['volume'] = 100;
        }else{
            status['audio'].volume(newVolume);
            status['volume'] = newVolume;
        }
    }
    res.status(200).send();
});

app.post('/dvolume', function(req, res){
    if(status['playing']){
        let volume = status['volume'];
        let newVolume = volume - 5;
        if(newVolume <= 0){
            status['audio'].volume(0);
            status['volume'] = 0;
        }else{
            status['audio'].volume(newVolume);
            status['volume'] = newVolume;
        }
    }
    res.status(200).send();
});


//advances playback by one song
app.post('/next', function (req, res) {
    //kill current song if it's playing
    if (status["playing"]) {
        status["audio"].stop();
    }

    //find the next song in the queue
    let nextSong = status['curSong'] + 1;
    if (nextSong >= songList.length) {
        nextSong = 0;
    }

    //play the new song and save state
    status['audio'].openFile("music/" + songList[nextSong]);
    status['audio'].play();

    //save other states
    status["playing"] = true;
    status['curSong'] = nextSong;

    res.status(200).send(songList[nextSong]);
});

//reverses playback by one song
app.post('/previous', function (req, res) {
    //kill current song if it's playing
    if (status["playing"]) {
        status["audio"].stop();
    }

    //find the next song in the queue
    let nextSong = status['curSong'] - 1;
    if (nextSong < 0) {
        nextSong = songList.length - 1;
    }

    //play the new song and save state
    status['audio'].openFile("music/" + songList[nextSong]);
    status['audio'].play();

    //save other states
    status["playing"] = true;
    status['curSong'] = nextSong;

    res.status(200).send(songList[nextSong]);
});

//kill the current song
app.post('/pause', function (req, res) {
    //if something is playing, stop it
    if (status["playing"]) {
        status["audio"].pause();
        status["playing"] = false;
    }
    res.sendStatus(200);
});

//return the song queue
app.get('/songs', function (req, res) {
    res.status(200).send(songList);
});