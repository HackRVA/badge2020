# Badge2020 Documentation
This is a static site built with [eleventy](https://www.11ty.dev/) that builds a documentation site to gh-pages.
Ideally we will put documentation here that is intended to be public facing.

## Adding Content

### Pages
You can add markdown files in the [./content/pages/](./content/pages/) directory.
These will show on the site as pages.

To get the page to link in the nav, you will need to include [frontmatter](https://www.11ty.dev/docs/data-frontmatter/) in the markdown file that looks something like this:

```md
---
title: Hello, World
date: Last Modified 
permalink: /hello_world.html
eleventyNavigation:
  key: hello
  order: 0
  title: Hello, World!
---

Hello, World!
```

### Images
Add images to the `./content/images/` dir.

gh-pages requires a pathPrefix.  
> i.e. the site may be served at `https://hackrva.github.io/badge2020/` the prefix would be `/badge2020`

I haven't figured out how to get this to play nicely with images yet. So, you will have to link the images in the markdown file with the path prefix included.
> e.g. `![Hello, world](/badge2020/content/images/hello.jpg)`

## Running the site locally
You will need to install node (probably something version 12 or higher) and npm.

### install the `node_modules` by running:
```
npm install
```

### run the site locally:
```
npm start
```
the site should be served up on `http://localhost:8080`

### deploy to gh-pages
```
npm run build
npm run deploy
```
