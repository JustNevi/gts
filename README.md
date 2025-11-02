# Git Transmit Storage

The tool is written in C that helps transfer messages through git repository.

## Usage

Roles: `TX`, `RX`.

`Alice` will have `TX` role. `Bob` will be `RX`.

### Initiation

`Alice` creates a git repository with the following files: `tx`, `rx`.

```
git init &&
touch tx rx &&
git add tx rx
```

```
git commit -m "init: init gts"
```

And pushes to remote git server:

```
git remote add origin git@example.com:/srv/git/gts.git
```

```
git push origin main
```

And detaches git HEAD in current commit:

```
git checkout main^0
```

`Bob` fetches `Alice` repo:

```
git init &&
git remote add origin git@example.com:/srv/git/gts.git &&
git pull origin main
```

And also detaches git HEAD:

```
git checkout main^0
```

### Conversation

`Alice` as `TX` can send and receive messages:

```
echo "Hi, Bob!" | gts tx send
```

```
echo "How are you doing?" | gts tx send
```

Also `Bob` as `RX`:

```
gts rx recv
# Output: Hi, Bob!
```

```
echo "Hello, Alice" | gts rx send
```

```
gts rx recv
# Output: How are you doing?
```

```
echo "I am doing well, thanks" | gts rx send
```

```
echo "What is new with you?" | gts rx send
```

`Alice` as `TX`:

```
gts tx recv
# Output: Hello, Alice
```

```
gts tx recv
# Output: I am doing well, thanks
```

```
gts tx recv
# Output: What is new with you?
```

```
gts tx recv
# No output. So this was the last message.
```
