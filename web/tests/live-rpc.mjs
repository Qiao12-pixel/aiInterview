// Optional integration: node web/tests/live-rpc.mjs against a mock-model server.
import assert from 'node:assert/strict';
import {accountRequest,createSession,streamAnswer,streamReport,listReports} from '../src/rpc/interview.js';
const base=process.env.INTERVIEW_TEST_URL;
if(!base)throw Error('Set INTERVIEW_TEST_URL to an isolated mock-model server');
const nativeFetch=globalThis.fetch;let cookie='';
globalThis.fetch=async(path,options={})=>{const response=await nativeFetch(base+path,{...options,headers:{...options.headers,Cookie:cookie}});const value=response.headers.get('set-cookie');if(value)cookie=value.split(';')[0];return response;};
const credentials={email:`js-${crypto.randomUUID()}@example.test`,password:'Synthetic-test-123',display_name:'JS integration'};
await accountRequest('register',credentials);
assert.equal((await accountRequest('me')).user.email,credentials.email);
function resumePdf() {
 const content='BT /F1 12 Tf 40 700 Td (Synthetic C++ developer resume. RAII and Boost.) Tj ET';
 const objects=['<< /Type /Catalog /Pages 2 0 R >>','<< /Type /Pages /Kids [3 0 R] /Count 1 >>','<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>','<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>',`<< /Length ${content.length} >>\nstream\n${content}\nendstream`];
 let pdf='%PDF-1.4\n';const offsets=[0];for(let i=0;i<objects.length;i++){offsets.push(pdf.length);pdf+=`${i+1} 0 obj\n${objects[i]}\nendobj\n`;}
 const xref=pdf.length;pdf+='xref\n0 6\n0000000000 65535 f \n'+offsets.slice(1).map(n=>String(n).padStart(10,'0')+' 00000 n \n').join('');pdf+=`trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n${xref}\n%%EOF\n`;return new TextEncoder().encode(pdf);
}
let session;const events=[];
await createSession({candidate_name:'测试候选人',question_total:1,resume_pdf:resumePdf()},m=>{events.push(m);if(m.type==='session')session=m;});
assert.ok(session.id);assert.ok(events.some(e=>e.event_type==='DELTA'));assert.ok(events.some(e=>e.event_type==='QUESTION'));
await streamAnswer({session_id:session.id,text:'RAII 利用对象生命周期管理资源。'},m=>events.push(m));
assert.ok(events.some(e=>e.event_type==='SUMMARY'));
let report;await streamReport({session_id:session.id},m=>{if(m.type==='report')report=m.report;});
assert.equal(report.session_id,session.id);assert.ok((await listReports()).length);
await assert.rejects(createSession({candidate_name:'Invalid PDF',question_total:1,resume_pdf:new TextEncoder().encode('not-pdf')},()=>{}));
await accountRequest('logout');await assert.rejects(listReports());
console.log('Browser JS wire integration passed: auth, first-question stream, answer, report, history, PDF upload/rejection, logout');
