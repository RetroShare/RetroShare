module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    watch: {
      // important: exclude node_modules
      files: ['**','!**/node_modules/**'],
	  options: {
	    livereload: true,
	  }
    }
  });
  grunt.loadNpmTasks('grunt-contrib-watch');
};