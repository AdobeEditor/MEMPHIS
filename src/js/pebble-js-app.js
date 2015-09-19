Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://rawgit.com/AdobeEditor/MEMPHIS/master/config/index.html';

  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));

  console.log('Configuration page returned: ' + JSON.stringify(configData));

  if (configData['skin_select']) {
	  dict['KEY_SKIN_SELECT'] = configData['skin_select']
  }

  Pebble.sendAppMessage(dict, function() {
      console.log('Send successful!' + JSON.stringify(dict));
  }, function() {
      console.log('Send failed!');
    });
});
