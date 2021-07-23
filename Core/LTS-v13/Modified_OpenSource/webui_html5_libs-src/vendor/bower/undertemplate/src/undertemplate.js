/*MIT license.
  https://github.com/silvermine/undertemplate
  This plugin has been customized as per our web framework.
*/
define(['jquery','underscore',"i18next"],function($,_,i18next) {
'use strict';
//var _ = require('underscore'),
  var  DEFAULT_SETTINGS, ESCAPE_ENTITIES;
var lang= i18next; //require("i18next");//lang plugin

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

/*This is function is used to eval language strings which are in the form of locale.t("") in all template html files.
This wont eval the model/variable data which are in template .html files.
path->This will contain all language string formats like local.t("common:title"),local.t('common:title') etc..
*/

function getValue(path, data) {  
  var final_lang_string="";
  if(path.indexOf("'") != -1 || path.indexOf('"') != -1){
    final_lang_string=(path.indexOf("'") != -1)?lang.t(path.split("'")[1]): lang.t(path.split('"')[1]);
  }
  return final_lang_string;
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
template=function(text,userSettings) { 
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
});
