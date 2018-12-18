# The **barber-shop** project

******

******

## Cloning the repo

### Approach 1

Just _git pull_ and then go to _barber-shop/bs-processes_ or _barber-shop/bs-threads_ directory.

### Approach 2

If you prefer to have a clean clone of only your subproject, do the following:
create a directory for your project, like _barber-shop_ for example; then

```

cd «directory-of-your-choice»

git init

git remote add origin https://code.ua.pt/git/«your-project-name»

git config core.sparseCheckout true

git config user.name "«your-name»"

git config user.email «your-email»@ua.pt

echo barber-shop/«your-subproject» > .git/info/sparse-checkout

git pull --depth 1 origin master

```

where «your-project-name» is your project name in _code.ua.pt_ and
«your-subproject» is either _bs-processes_ or _bs-threads_.

******

## Generating documentation

The code is documented in **doxygen**. So, you can easily generate **html** documentation pages.

```

cd «your-subproject»/doc

doxygen

firefox html/index.html &

```

Of course, you can change firefox by your favourite browser.

******

## Testing the code

```

cd «your-subproject»

make

./simulation

```

******


