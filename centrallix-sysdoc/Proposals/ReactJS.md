# Potential options for using React in Centrallix
Author: Alison Blomenberg

Date: July 2022

## Why use React?
Pros:
- React is one of the most popular modern JavaScript frameworks, which means there's a vibrant ecosystem of tools and tutorials around it, many people have had some experience with React, and further experience with React is generally desirable. This could potentially help with the roadblocks Centrallix and Kardia often face in recruiting and onboarding new developers. 
- React's fundamental model is JavaScript taking in a JSON-style data model and spitting out HTML, which then dynamically responds to changes in the data model (triggered by UI interactions, AJAX requests, etc). This means it's naturally good at interactivity, data-driven UIs, and separating data logic from display logic to make both clearer. Kardia is very data-driven, and sometimes struggles with the kind of speedy interactivity that React does well with its client-side dynamic rerendering model.
- React is naturally very modular, so it would lend itself to gradually building (or converting) small components and composing them.

Cons:
- React is typically used with a whole complex toolchain including npm, Babel, etc. This can be a lot to learn for a developer unfamiliar with the modern JS ecosystem, and is also a lot to integrate well into Centrallix (including security considerations with tools like npm).
- As with many tools in the JavaScript ecosystem, React best practices change quickly. It's also not "opinionated" about many details like how to manage state, styling, routing, etc, so while it is flexible, the ecosystem is fragmented with lots of different options and opinions on offer.
- Adding a new framework like this to Centrallix would make the existing system more complex and add more techologies that a Centrallix developer has to be familiar with to understand and work with the whole system.

## Standard client-side rendering
A "React component" is essentially some JavaScript code which ultimately generates HTML. In a typical React workflow, the web server just sents the client an HTML file with an empty "root" div and a JS file containing a bundle of React components. The React code executes on the client side, generates some HTML, and fills the empty div with it.

In theory, it would be pretty easy for Centrallix as it stands to embed React components like this. You'd just need an HTDriver that includes a React bundle JS as a script. However, there are a couple potential issues:
- Meaningful interoperation with Centrallix would probably be awkward. Initial parameters or data would either have to be awkwardly embedded in the HTML to be scraped by the React bundle, or would have be grabbed via a client-side AJAX request by the React bundle (which means the user has to wait for the page to initially load, then wait more for the AJAX request for the data to complete).
- All the HTML generation happens on the client side. This means more computing load on the client and more waiting for the component to initially render - which could be miniscule or could be problematic based on the complexity of the component that needs to be generated.
- Writing React code typically involves using a lot of syntactic sugar (such as JSX for writing HTML generation in a way that looks like HTML) that means you have to run it through a transpilation tool like Babel before you get a bundle JS that you can directly run in a browser. So that means more steps to development in Centrallix: write your React code, run all the transpilation, and then get your bundle.js that the HTDriver can include. (Obviously this could be handled automatically by a decent makefile, but it adds some more complexity and thus more opportunities for things to break.)

## Server-side rendering
React components can also be rendered into HTML by a Node server, on the server side. Essentially, the Node server just calls a React library function for turning a React component into HTML. This HTML can then be sent to the client by itself, or it can be sent along with a React bundle that "rehydrates" the generated HTML and then can respond and rerender dynamically like a normal React app. So you can use server-side rendering to produce static HTML, or to produce an initial speedier render of a React app.

Note that Node is not the only option for running JavaScript. There are also JS runtimes available such as [Deno](https://deno.land/), built with Rust, not to mention invoking V8 or SpiderMonkey directly. However, I don't know how well these other options might work with React and its typical ecosystem, so I will just assume Node use for now.

The key question for server-side rendering is, how do you get Centrallix talking to Node? Here are a couple different options from least complex (and dumbest) to most complex (and most customizable).

### Directly invoking a Node process
Theoretically, Centrallix could invoke Node the same way a C program might fork/exec or otherwise run any other process. You could pass in initial parameters and get generated HTML via stdin/stdout, etc. I don't know enough about handling processes in C to have a good idea of the major pros and cons of this approach. It sounds like it would be fairly easy to get initially working, but also pretty inflexible.

### Intercommunication with a separate Node server
Another option could be to have a Node server running in parallel to the Centrallix server, always available for requests by Centrallix to generate HTML from React code. You could use sockets (see https://github.com/RIAEvangelist/node-ipc) or some other standard protocol for communication between two servers.

### Embedding Node and using C APIs
The most complex option would be to embed Node within Centrallix and call out to it using its C APIs. Here is some documentation about that:

- https://nodejs.org/api/embedding.html
- https://v8.dev/docs/embed
- https://nodejs.org/api/n-api.html (this is designed for native addons to be invoked from Node, but can be used as an API for manipulating JS values, making calls to JS functions, etc. See for example https://github.com/nodejs/help/issues/1164)

This quickly gets complicated and it could be quite challenging to use the whole toolchain that React typically rests on (npm, Babel, etc). However, it would also allow for very in-depth interoperation between C and JS.
