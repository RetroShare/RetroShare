/*

  NoiseLoader Class jQuery Plugin definition
  
  Usage:
  

*/

!function ($) {
//
//  var template = '<div class="noise-loader-wrap">';
//      template += '<div class="label">';
//      template += 'Decrypting conversation secret from <span class="username">Tom</span>.<br/>';
//      template += 'This can happen when you start a chat or if the user has gone offline and rejoined.';
//      template += '</div>';
//      template += '<div class="percent-progress">0</div>';
//      template += '<div class="noise-loader"></div>';
//      template += '</div>';

  // NoiseLoader Class
  // --------------------------------------------

  NoiseLoader = function (element, options) {
    this.$container = $(element);

    // Defaults are controlled by element attributes
    options = $.extend({
      'percent'   : 0,
      'template' : options.template
    }, options);

    this.options = options;
    this.percent = options.percent;

    this.$container.prepend(this.options.template);
    this.$ele = this.$container.find('.noise-loader');

    this.initialize();
  };


  NoiseLoader.prototype.initialize = function(selector) {
    this.createBlocks();
    this.setupRandomOpacity();
  };

  NoiseLoader.prototype.createBlocks = function() {
    var height = this.$ele.height();
    var width = this.$ele.width();
    var blockWidthHeight = 4;

    // Create blocks
    for (var i = 0; i < width; i+=blockWidthHeight) {
      for (var j = 0; j < height; j+=blockWidthHeight) {
        this.$ele.append('<div></div>');
      };
    }
    this.$blocks = this.$ele.find('div');
  };

  NoiseLoader.prototype.setupRandomOpacity = function() {
    this.blockData = [];
    var blockData = this.blockData;

    this.$blocks.each(function(){
      blockData.push({
        $block: $(this),
        opacity: Math.random() - 1,
        velocity: Math.random() - .5
      });

      $(this).css({opacity: blockData[blockData.length-1].opacity});
    });
  };


  NoiseLoader.prototype.updateRandomOpacity = function() {
    if (!this.blockData) return;
    var currentOpacity = 1;

    for (var i = 0; i < this.blockData.length; i++) {
      currentOpacity = this.blockData[i].opacity + this.percent*2;
      // if (i == 3) console.log(this.blockData[i].opacity, this.percent, currentOpacity);
      this.blockData[i].$block.css({opacity:currentOpacity});
    };
  };

  NoiseLoader.prototype.finishAnimation = function() {
    this.updateRandomOpacity();
    this.$ele.html('<div class="complete"></div>');
    this.$ele.addClass('complete');
    var thisNoiseLoader = this;
    var myTimeOut=setTimeout(function(){
      thisNoiseLoader.$ele.parents('.noise-loader-wrap').fadeOut('fast', function(){
        $(this).detach();
        thisNoiseLoader.selfDestruct();
      });
	  clearTimeout(myTimeOut);
    },500);
  };

  NoiseLoader.prototype.setPercent = function(value) {
 
    var options = {
      duration:100, 
      step: this.updateRandomOpacity
    };
    if (value == 1) options.complete = this.finishAnimation;  
		// Update percent text
		this.$ele.siblings('.percent-progress').text(Math.floor(value*100)); // + '%');
		 // Animate the noise 
		$(this).stop().animate({percent: value}, options);
		 
	 
  };

  NoiseLoader.prototype.selfDestruct = function() {
    this.$container.data('noiseLoader', null);
  }


  // jQuery Plugin - for DOM access
  // --------------------------------------------

  $.fn.noiseLoader = function(options, value) {
	
//	console.log('options.template ------ ' + options.template);
	  
    return this.each(function() {
      var $this = $(this),
        data = $this.data('noiseLoader');

      // initialize if not already 
      if (!data) {
        data = new NoiseLoader(this, options);
        $this.data('noiseLoader', data);
      }

      if (typeof options === 'string' && typeof data[options] == 'function') {
        // This could be updated to accommodate method arguments
        data[options](value);

      }
    });
  }


}(window.jQuery);