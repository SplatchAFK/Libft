/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: romasant <romasant@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/18 13:45:23 by romasant          #+#    #+#             */
/*   Updated: 2026/10/21 22:29:33 by romasant         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

static void	ft_free_all_tab(char **s1, size_t count_word)
{
	while (count_word--)
		free(s1[count_word]);
	free(s1);
}

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

static size_t	ft_fill_tab(char **big_str, char const *s1, char c)
{
	size_t	i;
	size_t	count;

	i = 0;
	while (*s1)
	{
		while (*s1 == c)
			s1++;
		count = 0;
		while (s1[count] && s1[count] != c)
			count++;
		if (count > 0)
		{
			big_str[i] = ft_substr(s1, 0, count);
			if (big_str[i] == NULL)
				return (ft_free_all_tab(big_str, i), 0);
			i++;
		}
		s1 += count;
	}
	big_str[i] = NULL;
	return (1);
}

char	**ft_split(char const *s1, char c)
{
	size_t	count_word;
	char	**big_str;

	count_word = ft_count_word(s1, c);
	big_str = malloc(sizeof(char *) * (count_word + 1));
	if (big_str == NULL)
		return (NULL);
	if (!ft_fill_tab(big_str, s1, c))
		return (NULL);
	return (big_str);
}
