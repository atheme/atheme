/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_argstack.c: Argument stacks.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "mowgli.h"

static mowgli_object_class_t klass;

/*
 * \brief Private destructor for the mowgli_argstack_t object.
 *
 * \param vptr pointer to mowgli_argstack_t to destroy.
 */
static void mowgli_argstack_destroy(void *vptr)
{
	mowgli_argstack_t *self = (mowgli_argstack_t *) vptr;
	mowgli_node_t *n, *tn;

	MOWGLI_LIST_FOREACH_SAFE(n, tn, self->stack.head)
	{
		mowgli_free(n->data);

		mowgli_node_delete(n, &self->stack);
		mowgli_node_free(n);
	}

	mowgli_free(self);
}

/*
 * \brief Initialization code for the mowgli.argstack library.
 * 
 * Side Effects:
 *       - the mowgli_argstack_t object class is registered.
 */
void mowgli_argstack_init(void)
{
	mowgli_object_class_init(&klass, "mowgli_argstack_t", mowgli_argstack_destroy, FALSE);
}

/*
 * \brief Creates an argument stack from a va_list and an appropriate 
 * description schema.
 *
 * \param descstr a description string which describes the argument stack, where:
 *   + the character 's' means that the value for that slot is a string
 *   + the character 'd' means that the value for that slot is a numeric
 *   + the character 'p' means that the value for that slot is a generic pointer
 *   + the character 'b' means that the value for that slot is a boolean
 * \param va a va_list containing data to populate the argument stack with.
 *
 * \return a mowgli_argstack_t (mowgli.argstack) object.
 */
mowgli_argstack_t *mowgli_argstack_create_from_va_list(const char *descstr, va_list va)
{
	const char *cp = descstr;
	mowgli_argstack_t *out = mowgli_alloc(sizeof(mowgli_argstack_t));
	mowgli_object_init(mowgli_object(out), descstr, &klass, NULL);

	if (descstr == NULL)
		mowgli_throw_exception_val(mowgli.argstack.invalid_description, NULL);

	while (*cp)
	{
		mowgli_argstack_element_t *e = mowgli_alloc(sizeof(mowgli_argstack_element_t));

		switch(*cp)
		{
		case 's':
			e->data.string = va_arg(va, char *);
			e->type = MOWGLI_ARG_STRING;
			break;
		case 'd':
			e->data.numeric = va_arg(va, int);
			e->type = MOWGLI_ARG_NUMERIC;
			break;
		case 'p':
			e->data.pointer = va_arg(va, void *);
			e->type = MOWGLI_ARG_POINTER;
			break;
		case 'b':
			e->data.boolean = va_arg(va, mowgli_boolean_t);
			e->type = MOWGLI_ARG_BOOLEAN;
			break;
		default:
			va_end(va);
			mowgli_object_unref(out);
			mowgli_throw_exception_val(mowgli.argstack.invalid_description, NULL);
			break;
		}

		mowgli_node_add(e, mowgli_node_create(), &out->stack);
		cp++;
	}

	return out;
}

/*
 * \brief Creates an argument stack.
 *
 * \param descstr a description string which describes the argument stack, where:
 *   + the character 's' means that the value for that slot is a string
 *   + the character 'd' means that the value for that slot is a numeric
 *   + the character 'p' means that the value for that slot is a generic pointer
 *   + the character 'b' means that the value for that slot is a boolean
 * \param va a va_list containing data to populate the argument stack with.
 *
 * \return a mowgli_argstack_t (mowgli.argstack) object.
 */
mowgli_argstack_t *mowgli_argstack_create(const char *descstr, ...)
{
	va_list va;
	mowgli_argstack_t *out;

	if (descstr == NULL)
		mowgli_throw_exception_val(mowgli.argstack.invalid_description, NULL);

	va_start(va, descstr);
	out = mowgli_argstack_create_from_va_list(descstr, va);
	va_end(va);

	return out;
}

/*
 * \brief Convenience function to pop a string value off of an argument stack.
 *
 * \param self A mowgli_argstack_t object to pop a string off of.
 *
 * \return On success, a string.
 *
 * Side Effects:
 *       - the argument is removed from the argstack.
 */
const char *mowgli_argstack_pop_string(mowgli_argstack_t *self)
{
	mowgli_node_t *n;
	mowgli_argstack_element_t *e;

	if (self == NULL)
		mowgli_throw_exception_val(mowgli.null_pointer_exception, NULL);

	n = self->stack.head;
	mowgli_node_delete(n, &self->stack);
	e = n->data;
	mowgli_node_free(n);

	return e->data.string;
}

/*
 * \brief Convenience function to pop a numeric value off of an argument stack.
 *
 * \param self A mowgli_argstack_t object to pop a numeric off of.
 *
 * \return On success, a numeric.
 *
 * Side Effects:
 *       - the argument is removed from the argstack.
 */
int mowgli_argstack_pop_numeric(mowgli_argstack_t *self)
{
	mowgli_node_t *n;
	mowgli_argstack_element_t *e;

	if (self == NULL)
		mowgli_throw_exception_val(mowgli.null_pointer_exception, 0);

	n = self->stack.head;
	mowgli_node_delete(n, &self->stack);
	e = n->data;
	mowgli_node_free(n);

	return e->data.numeric;
}

/*
 * Convenience function to pop a boolean value off of an argument stack.
 *
 * \param self A mowgli_argstack_t object to pop a boolean off of.
 *
 * \return On success, a boolean value.
 *
 * Side Effects:
 *       - the argument is removed from the argstack.
 */
mowgli_boolean_t mowgli_argstack_pop_boolean(mowgli_argstack_t *self)
{
	mowgli_node_t *n;
	mowgli_argstack_element_t *e;

	if (self == NULL)
		mowgli_throw_exception_val(mowgli.null_pointer_exception, FALSE);

	n = self->stack.head;
	mowgli_node_delete(n, &self->stack);
	e = n->data;
	mowgli_node_free(n);

	return e->data.boolean;
}

/*
 * \brief Convenience function to pop a pointer value off of an argument stack.
 *
 * \param self A mowgli_argstack_t object to pop a pointer off of.
 *
 * \return On success, a pointer.
 *
 * Side Effects:
 *       - the argument is removed from the argstack.
 */
void *mowgli_argstack_pop_pointer(mowgli_argstack_t *self)
{
	mowgli_node_t *n;
	mowgli_argstack_element_t *e;

	if (self == NULL)
		mowgli_throw_exception_val(mowgli.null_pointer_exception, NULL);

	n = self->stack.head;
	mowgli_node_delete(n, &self->stack);
	e = n->data;
	mowgli_node_free(n);

	return e->data.pointer;
}
