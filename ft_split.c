/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: romasant <romasant@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/18 13:45:23 by romasant          #+#    #+#             */
/*   Updated: 2026/10/21 20:44:19 by romasant         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

static size_t	ft_count_word(char const *s1, char c)
{
	int		in_word;
	size_t	counter_w;

	in_word = 0;
	counter_w = 0;
	while (*s1)
	{
		if (in_word == 0 && *s1 != c)
		{
			counter_w++;
			in_word = 1;
		}
		else if (*s1 == c)
			in_word = 0;
		s1++;
	}
	return (counter_w);
}

char	**ft_split(char const *s1, char c)
{
	size_t	count;
	size_t	i;
	char	**big_str;

	big_str = malloc(sizeof(char *) * ft_count_word(s1, c) + 1);
	if (big_str == NULL)
		return (NULL);
	i = 0;
	while (*s1)
	{
		while (*s1 == c)
			s1++;
		count = 0;
		while (s1[count] && s1[count] != c)
			count++;
		if (count > 0)
			big_str[i++] = ft_substr(s1, 0, count);
		s1 += count;
	}
	big_str[i] = NULL;
	return (big_str);
}
