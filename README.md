# Charlie's Ruby

Charlie's Ruby is a fork of [MRI Ruby](http://github.com/ruby/ruby) with a focus on experimental features.

## Differences

### References

You can take references to locals, instance variables, class variables and global variables in Charlie's Ruby with the backslash operator. This returns a `Ref` object.

Example:

```ruby
a = 123
b = \a

p b.value # => 123

b.value = 456
p a # => 456
```
