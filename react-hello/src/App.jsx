import { useState } from 'react'
import reactLogo from './assets/react.svg'
import viteLogo from '/vite.svg'
import './App.css'
import * as Juce from "./js/juce/index.js";

console.log("JUCE frontend library successfully imported");
const nativeFunction = Juce.getNativeFunction("sayHelloToJuce");

function showNotificationUI(message) {
    const toast = document.createElement("div");
    toast.className = "toast";
    toast.innerText = `Notification: ${message}`;
    document.body.appendChild(toast);
    setTimeout(() => toast.remove(), 30000);
    alert("FOO: " + message);
}

window.__JUCE__.backend.addEventListener("exampleEvent", (objectFromCppBackend) => {
    console.log("Received an event!");
    console.log(objectFromCppBackend);
    showNotificationUI(objectFromCppBackend);
});

function App() {
  const [count, setCount] = useState(0)

  return (
    <>
      <div>
        <a href="https://vite.dev" target="_blank">
          <img src={viteLogo} className="logo" alt="Vite logo" />
        </a>
        <a href="https://react.dev" target="_blank">
          <img src={reactLogo} className="logo react" alt="React logo" />
        </a>
      </div>
      <h1>Vite + React</h1>
      <div className="card">
        <button onClick={() => setCount((count) => count + 1)}>
          count is {count}
        </button>
        <button onClick={() => {
          const response = nativeFunction("Test message");
          console.log("Response from C++:", response);
        }}>
          Call native code
        </button>
        <p>
          Edit <code>src/App.jsx</code> and save to test HMR
        </p>
      </div>
      <p className="read-the-docs">
        Click on the Vite and React logos to learn more
      </p>
    </>
  )
}

export default App
