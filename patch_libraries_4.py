path = '/data/data/com.termux/files/home/seL4-workspace/libraries-4/main.c'
with open(path, 'r') as f:
    code = f.read()

# Fix VSpace root for x86_64
code = code.replace(
    'simple_get_pd(&simple)',
    'seL4_CapInitThreadVSpace'
)

# Task 1: Allocate notification object
code = code.replace(
    '    vka_object_t ntfn_object = {0};',
    '''    vka_object_t ntfn_object = {0};
    error = vka_alloc_notification(&vka, &ntfn_object);
    assert(error == 0);'''
)

# Task 2: Initialize default timer
code = code.replace(
    '    /* TASK 2: call ltimer library to get the default timer */\n    /* hint: ltimer_default_init, you can set NULL for the callback and token\n     */',
    '''    /* TASK 2: call ltimer library to get the default timer */
    error = ltimer_default_init(&timer, ops, NULL, NULL);
    assert(error == 0);'''
)

# Task 3: Configure and start periodic timer
code = code.replace(
    '    /*\n     * TASK 3: Start and configure the timer\n     * hint 1: ltimer_set_timeout\n     * hint 2: set period to 1 millisecond\n     */',
    '''    /* TASK 3: Start and configure the timer */
    error = ltimer_set_timeout(&timer, 1 * NS_IN_MS, TIMEOUT_PERIODIC);
    assert(error == 0);'''
)

# Task 4: Handle timer interrupt
code = code.replace(
    '        /*\n         * TASK 4: Handle the timer interrupt\n         * hint 1: wait for the incoming interrupt and handle it\n         * The loop runs for (1000 * msg) times, which is basically 1 second * msg.\n         *\n         * hint2: seL4_Wait\n         * hint3: sel4platsupport_irq_handle\n         * hint4: \'ntfn_id\' should be MINI_IRQ_INTERFACE_NTFN_ID and handle_mask\' should be the badge\n         *\n         */',
    '''        /* TASK 4: Handle the timer interrupt */
        seL4_Word badge = 0;
        seL4_Wait(ntfn_object.cptr, &badge);
        sel4platsupport_irq_handle(&ops.irq_ops, MINI_IRQ_INTERFACE_NTFN_ID, badge);'''
)

# Task 5: Stop timer
code = code.replace(
    '    /*\n     * TASK 5: Stop the timer\n     * hint: ltimer_destroy\n     */',
    '''    /* TASK 5: Stop the timer */
    ltimer_destroy(&timer);'''
)

with open(path, 'w') as f:
    f.write(code)

print("Applied libraries-4 patch.")
