'use strict';

// We simply require the specific functions we need from lodash to create a smaller bundle
// in browserify-style environments.
/* eslint-disable global-require */
module.exports = {
   get: require('lodash/get'),
   keys: require('lodash/keys'),
   trim: require('lodash/trim'),
   each: require('lodash/each'),
   reduce: require('lodash/reduce'),
   defaults: require('lodash/defaults'),
   isFunction: require('lodash/isFunction'),
};
