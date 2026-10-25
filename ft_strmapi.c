/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strmapi.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: romasant <romasant@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/25 18:27:26 by romasant          #+#    #+#             */
/*   Updated: 2026/10/25 18:38:08 by romasant         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strmapi(char const *s, char (*f) (unsigned int, char))
{
	char			*s_new;
	unsigned int	counter;

	s_new = malloc(sizeof(char) * ft_strlen(s) + 1);
	if (s_new == NULL)
		return (NULL);
	counter = 0;
	while (s[counter] != '\0')
	{
		s_new[counter] = f(counter, s[counter]);
		counter++;
	}
	s_new[counter] = '\0';
	return (s_new);
}
