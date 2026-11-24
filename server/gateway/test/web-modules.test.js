import test from 'node:test';
import assert from 'node:assert/strict';
import {streamAnswer} from '../../../web/src/sse.js';
import {createVoice} from '../../../web/src/voice.js';
import {createCamera} from '../../../web/src/camera.js';

test('browser SSE decoder handles UTF-8 bytes and rejects truncated streams',async()=>{
 const original=globalThis.fetch;
 const wire='data: '+JSON.stringify({type:'event',event_type:'DELTA',text:'中文流式输出'})+'\n\ndata: {"type":"done"}\n\n';
 function response(text){const bytes=new TextEncoder().encode(text);return new Response(new ReadableStream({start(controller){for(const byte of bytes)controller.enqueue(Uint8Array.of(byte));controller.close();}}));}
 try{
  globalThis.fetch=async()=>response(wire);const messages=[];
  await streamAnswer({},event=>messages.push(event));assert.equal(messages[0].text,'中文流式输出');
  globalThis.fetch=async()=>response('data: {"type":"status"}\n\n');
  await assert.rejects(streamAnswer({},()=>{}),/输出流中断/);
 }finally{globalThis.fetch=original;}
});
test('voice keeps transcription editable and releases recognition on dispose',()=>{
 const previous=globalThis.window;let instance,aborted=false,cancelled=false;
 class Recognition{constructor(){instance=this;}start(){this.onstart();}stop(){this.onend();}abort(){aborted=true;}}
 globalThis.window={SpeechRecognition:Recognition,speechSynthesis:{cancel(){cancelled=true;}}};
 try{
  const input={value:'初稿：'},voice=createVoice({input,onStatus(){},onChange(){}});
  voice.start();assert.equal(voice.listening(),true);
  instance.onresult({resultIndex:0,results:[Object.assign([{transcript:'资源自动释放'}],{isFinal:true})]});
  assert.equal(input.value,'初稿：资源自动释放');voice.stop();assert.equal(voice.listening(),false);
  voice.dispose();assert.ok(aborted&&cancelled);
 }finally{globalThis.window=previous;}
});
test('camera handles denied permission and stops all local tracks',async()=>{
 const previous=Object.getOwnPropertyDescriptor(globalThis,'navigator');let stopped=0,denied=true;
 const track={stop(){stopped++;}},video={},placeholder={hidden:false},status=[];
 Object.defineProperty(globalThis,'navigator',{configurable:true,value:{mediaDevices:{async getUserMedia(){if(denied)throw Error('denied');return {getTracks:()=>[track],getVideoTracks:()=>[track]};}}}});
 try{
  const camera=createCamera({video,placeholder,onStatus:text=>status.push(text),onChange(){}});
  await camera.toggle();assert.equal(camera.active(),false);assert.match(status.at(-1),/权限/);
  denied=false;await camera.toggle();assert.equal(camera.active(),true);camera.stop();assert.equal(stopped,1);assert.equal(video.srcObject,null);
 }finally{if(previous)Object.defineProperty(globalThis,'navigator',previous);else delete globalThis.navigator;}
});
