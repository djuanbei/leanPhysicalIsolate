-- Sample 10: type class
class HasZero (α : Type) where
  zero : α
instance : HasZero Nat where
  zero := 0
#check (HasZero.zero : Nat)