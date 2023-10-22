# Smart Pointers

### ```UniquePtr```

   * Реализовал базовую функциональность ```UniquePtr```.
   * Добавил ```DefaultDeleter``` (функтор, который вызывается для объекта в 
   деструкторе указателя).
   * Интегрировал ```CompressedPair``` для ```DefaultDeleter```.
   * Специализировал шаблон для массивов --- ```UniquePtr<T[]>```.

### ```SharedPtr```

   * Реализовал базовую функциональность ```SharedPtr```.
   * Добавил оптимизированный ```MakeShared``` (одна аллокация на 
   контрольный блок и элемент).

### ```WeakPtr```

Реализовал ```WeakPtr``` - младшего брата ```SharedPtr```.

### ```Shared From This```

Реализовал ```EnableSharedFromThis``` - способ создать ```SharedPtr```,
   имея лишь ```this```.

### ```IntrusivePtr```

   * Реализовал базовую функциональность ```IntrusivePtr```.
   * Добавил удобную функцию ```MakeIntrusive```.
