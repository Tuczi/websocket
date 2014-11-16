/**
 *
 * @param info - fields: width, height, scale, frequency
 */
function Player(canvas, info) {
  this.canvas = canvas;
  this.info = info;
  this.ctx = canvas.getContext('2d');
  this.imageData = this.ctx.createImageData(info.width, info.height);
  this.frameBuffer = [];
  this.isPlaying = false;
  this.interval = null;
  this.willBeOtherFrames = true;

  canvas.style.width = info.scale*info.width + "px";
  canvas.style.height = info.scale*info.height + "px";

  this.play = function() {
    if(this.isPlaying) return;

    this.isPlaying = true;
    var that = this;
    this.interval = setInterval(function(){
      that.drawFrame();
      if(!that.willBeOtherFrames && that.frameBuffer.length <=0)
        that.reset();
    }, 1000/that.info.frequency);//TODO Math.round?
  };

  this.addFrame = function(data) {
   this.frameBuffer.push(data);
  };

  this.drawFrame = function() {
    if(this.frameBuffer <=0) return;

    var data = this.frameBuffer.shift();
    var imageData = this.ctx.createImageData(info.width, info.height);

    for (var i = 0, j=0; i < info.width*info.height*3; i+=3,j+=4) { // TODO have to copy?
      imageData.data[j] = data[i+0];
      imageData.data[j+1] = data[i+1];
      imageData.data[j+2] = data[i+2];
      imageData.data[j+3] = 255;
    }
    this.ctx.putImageData(imageData, 0, 0);
  };

  this.reset = function() {
    clearInterval(this.interval);
    this.interval=null;

    isPlaying=false;
  };

  this.endOfFrames = function() {
    this.willBeOtherFrames = false;
  }
};
