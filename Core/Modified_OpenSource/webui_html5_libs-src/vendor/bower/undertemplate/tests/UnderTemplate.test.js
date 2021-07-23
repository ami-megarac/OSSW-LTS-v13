'use strict';

var expect = require('chai').expect,
    makeTemplate = require('../src');

describe('UnderTemplate', function() {

   it('returns a function when making a template', function() {
      expect(makeTemplate('Hello <%= name %>')).to.be.a('function');
   });

   function one(tmpl, exp, data) {
      var compiled = makeTemplate(tmpl);

      expect(compiled(data)).to.eql(exp);
   }

   function testSuite(t, p, exp) {
      it('works with single simple value', function() {
         one('Hello ' + t + ' name %>', exp, {
            name: (p.firstName + ' ' + p.lastName),
         });
      });
      it('works with multiple simple values', function() {
         one('Hello ' + t + ' firstName %> ' + t + ' lastName %>', exp, p);
      });
      it('works with nested objects', function() {
         one('Hello ' + t + ' person.firstName %> ' + t + ' person.lastName %>', exp, { person: p });
      });
      it('works with arrays', function() {
         one('Hello ' + t + ' people[0].firstName %> ' + t + ' people[0].lastName %>', exp, { people: [ p ] });
      });
      it('works with deeply nested objects', function() {
         one('Hello ' + t + ' data.people[0].firstName %> ' + t + ' data.people[0].lastName %>', exp, { data: { people: [ p ] } });
      });
   }

   describe('basic operation', function() {
      testSuite('<%=', { firstName: 'John', lastName: 'Smith' }, 'Hello John Smith');
   });

   describe('does not escape interpolated', function() {
      testSuite('<%=', { firstName: 'John<br>', lastName: 'Smith' }, 'Hello John<br> Smith');
   });

   describe('escapes interpolated', function() {
      testSuite('<%-', { firstName: 'John<br>', lastName: 'Smith' }, 'Hello John&lt;br&gt; Smith');
   });

});
