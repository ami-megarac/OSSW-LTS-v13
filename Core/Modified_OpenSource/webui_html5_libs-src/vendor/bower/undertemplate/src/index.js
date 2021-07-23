'use strict';

var _ = require('./lodash'), // our local, smaller version
    DEFAULT_SETTINGS, ESCAPE_ENTITIES;

DEFAULT_SETTINGS = {
   escape: /<%-([\s\S]+?)%>/g,
   interpolate: /<%=([\s\S]+?)%>/g,
};

ESCAPE_ENTITIES = {
   '&': '&amp;',
   '<': '&lt;',
   '>': '&gt;',
   '"': '&quot;',
   "'": '&#x27;', // eslint-disable-line quotes
   '`': '&#x60;',
};

function getValue(path, data) {
   return _.get(data, _.trim(path), '');
}

function escapeHTML(str) {
   var pattern = '(?:' + _.keys(ESCAPE_ENTITIES).join('|') + ')',
       testRegExp = new RegExp(pattern),
       replaceRegExp = new RegExp(pattern, 'g');

   if (testRegExp.test(str)) {
      return str.replace(replaceRegExp, function(match) {
         return ESCAPE_ENTITIES[match];
      });
   }

   return str;
}

module.exports = function(text, userSettings) {
   var parts = [],
       index = 0,
       settings = _.defaults({}, userSettings, DEFAULT_SETTINGS),
       regExpPattern, matcher;

   regExpPattern = [
      settings.escape.source,
      settings.interpolate.source,
   ];
   matcher = new RegExp(regExpPattern.join('|') + '|$', 'g');

   text.replace(matcher, function(match, escape, interpolate, offset) {
      parts.push(text.slice(index, offset));
      index = offset + match.length;

      if (escape) {
         parts.push(function(data) {
            return escapeHTML(getValue(escape, data));
         });
      } else if (interpolate) {
         parts.push(getValue.bind(null, interpolate));
      }
   });

   return function(data) {
      return _.reduce(parts, function(str, part) {
         return str + (_.isFunction(part) ? part(data) : part);
      }, '');
   };
};
