document.addEventListener('DOMContentLoaded', init, false);

/** 
* You can manipulate the video here
* For example: Uncomment the code below and in the index to get a Start/Stop button
*/
function init() {
  const VP = document.getElementById('videoPlayer')
  const VPToggle = document.getElementById('toggleButton')

  VPToggle.addEventListener('click', function() {
    if (VP.paused) VP.play()
    else VP.pause()
  })
}

function refreshIt(element) {
  setTimeout(function() {
      element.src = element.src.split('?')[0] + '?' + new Date().getTime();
      refreshIt(element);;
      refreshIt(element);
  }, 500); // refresh every 0.5s
}